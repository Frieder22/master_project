import numpy as np
import json
import random

#PATH = "performance/"     # used for using vsCODE debugger
PATH = ""

NEXPERIMENTRUNS = 30
BIGSETSIZE = 10_000_000
BIGFILLING = 100_000
SHOWOUTPUT = False


MPI_LOAD = {"openMPI": "spack load openmpi@4.1.6",
           "intelMPI": "module load mpi/intelmpi",
           "MPICH": "spack load mpich@4.1.2"
           }

MPI_UNLOAD = {"openMPI": "spack unload openmpi@4.1.6 ",
           "intelMPI": "module unload mpi/intelmpi",
           "MPICH": "spack unload mpich@4.1.2 "
           }

MPI_LAUNCHER = {"openMPI": "srun",
           "intelMPI": "srun",
           "MPICH": "srun"
           }

CRITSIZE_STRAT = {"max": 0,
                  "min": 1,
                  "mean": 2
}

def createRunsJSON_BFS(data, tn):
    if type(data["graph"]) != list:
        data["graph"] = [data["graph"]]
    if type(data["func"]) != list:
        data["func"] = [data["func"]]
    if type(data["rootNode"]) != list:
        data["rootNode"] = [data["rootNode"]]

    runList = []
    for func in data["func"]:
        for graph in data["graph"]:
            for rootNode in data["rootNode"]:
                runList.append((func, graph, rootNode)) 

    # shuffle runs
    random.seed(42)
    random.shuffle(runList)

    # create run.json file
    outJSON = {
        "bfs": True,
        "runs": [
            {"graphPath": graph,
            "func": func,
            "nRepetitions": data["nRepetitions"],
            "rootNode": rootNode
            }
            for func, graph, rootNode in runList
        ]
    }
    f = open(PATH + "runs.json", "w")
    json.dump(outJSON, f, indent=4)
    f.close()

def createRunsJSON(data, tn):
    if "bfs" in data:
        createRunsJSON_BFS(data, tn)
        return
    def useNormalRepetitions(setSize, filling, vsFunc, funcType):
        if funcType == "Vertexset":
            if vsFunc in ["ALLREDUCE_BUTTERFLY", "ALLREDUCE_BUTTERFLY_REVERSE", "ALLREDUCE_SHIFT"]:
                return setSize <= BIGSETSIZE
            if vsFunc in ["ALLGATHER_BUTTERFLY", "ALLGATHER_BUTTERFLY_REVERSE", "ALLGATHER_SHIFT"]:
                return filling <= BIGFILLING
            return not((filling >= BIGFILLING or filling<0) and setSize >= BIGSETSIZE)
        if funcType == "Allreduce":
            return setSize <= BIGSETSIZE
        if funcType == "Allgather":
            return filling <= BIGFILLING


    # change all listable keys into lists
    for key in ["funcType", "vsFunc", "setSize", "filling"]:
        if type(data[key]) != list:
            data[key] = [data[key]]

    # add default if neccesary
    if "critsizeStrategies" in data:
        if type(data["critsizeStrategies"]) != list:
            data["critsizeStrategies"] = [data["critsizeStrategies"]]
    else:
        data["critsizeStrategies"] = ["max"]

    # find all run setups
    runList = []
    for critStrat in data["critsizeStrategies"]:
        for funcType in data["funcType"]:
            for setSize in data["setSize"]:
                for filling in data["filling"]:
                    if funcType == "Vertexset":
                        for vsFunc in data["vsFunc"]:
                            runList.append((funcType, vsFunc, setSize, filling, critStrat))
                    else:
                        runList.append((funcType, "ITERATOR", setSize, filling, critStrat)) # using ITERATOR as dummy

    # define set filling distribution
    distr = "uniform"
    if "elementDistribution" in data:
        distr = data["elementDistribution"]

    # shuffle runs
    random.seed(42)
    random.shuffle(runList)

    # create run.json file
    outJSON = {
        "runs": [
            {"experiment": data["experiment"],
            "funcType": funcType,
            "vsFunc": tn[vsFunc],
            "setSize": setSize,
            "filling": filling,
            "nRepetitions": data["nRepetitions"] if useNormalRepetitions(setSize, filling, vsFunc, funcType) else data["nRepetitionsSmall"],
            "distribution": distr,
            "critSizeStrat": CRITSIZE_STRAT[critStrat]}
            for funcType, vsFunc, setSize, filling, critStrat in runList
        ]
    }
    f = open(PATH + "runs.json", "w")
    json.dump(outJSON, f, indent=4)
    f.close()

def createSlurmJob(data, experimentName):
    # prepare and get setup values
    nNodes = data["nNodes"]
    perNode = data["perNode"]
    if type(data["mpilib"]) != list:
        data["mpilib"] = [data["mpilib"]]

    f = open("run.job", "w")
    # setup on cluster
    f.write("#! /bin/bash\n")
    f.write("#SBATCH -p q_thesis\n")
    f.write("#SBATCH -N {}\n".format(nNodes))
    f.write("#SBATCH --ntasks-per-node={}\n".format(perNode))
    f.write("#SBATCH --cpu-freq=High\n")
    f.write("#SBATCH --job-name={}\n".format(experimentName))
    f.write("#SBATCH --time=1:00:00\n\n")

    #create folder for saved data
    f.write("mkdir ../data/{0}\n\n".format(experimentName))
    # procedure for different MPI libs
    for mpilib in data["mpilib"]:
        # delete old measurement data, if there is some present
        f.write("rm ../data/{0}/{1}x{2}{3}.txt\n".format(experimentName,
                                                nNodes,
                                                perNode,
                                                mpilib))
        
        f.write("echo MPIlib: {0}\n".format(mpilib))
        f.write(MPI_LOAD[mpilib] +  "\n")
        f.write("make perf\n")
        f.write("for i in {{1..{0}}}; do\n".format(NEXPERIMENTRUNS))

        if SHOWOUTPUT:
            f.write("    {0} ../build/executeExperimentRuns.o runs.json \n".format(MPI_LAUNCHER[mpilib]))
        else:
            f.write("    {0} ../build/executeExperimentRuns.o runs.json >> ../data/{1}/{2}x{3}{4}.txt\n".format(MPI_LAUNCHER[mpilib],
                                                                                                     experimentName,
                                                                                                     nNodes,
                                                                                                     perNode,
                                                                                                     mpilib))
        f.write("echo iteration $i\n")
        f.write("done\n")
        f.write(MPI_UNLOAD[mpilib] + "\n\n")

    f.write("echo finished\n")

    f.close()

def findTestNumber():
    # do some shananigans to find the correct enum values from "testStructure.h"
    testnumber = {}
    with open(PATH + "../vertexSet/testStructure.h", "r") as f:
        line = f.readline()
        while line != "enum Testcase{\n":
            line = f.readline()
        count = 0
        line = f.readline()
        while line != "};\n":
            word = line.split(",")[0].replace(" ", "").replace("\n","")
            testnumber[word] = count
            count += 1
            line = f.readline()
    return testnumber


# load setup data
f = open(PATH + "experiments/sheduledExperiment.txt")
pathExperiment = f.readline()
f.close()

experiment = pathExperiment.split("/")[0]

f = open(PATH + "experiments/" + pathExperiment)
toExecuteData = json.load(f)
f.close()

# find correct IDs for Vertexsetfunctions
tn = findTestNumber()

createRunsJSON(toExecuteData, tn)
createSlurmJob(toExecuteData, experiment)
