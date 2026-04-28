import json
import subprocess
import os
import sys
import matplotlib.pylab as plt
import numpy as np
import argparse

PATH = ""
DPI = 300

ALLGATHER_TYPE = 0
ALLREDUCE_TYPE = 1
VERTEXSET_TYPE = 2

plotsizes = {"small": (4,4),
            "medium": (6,4)}

parser = argparse.ArgumentParser(prog="plotExperiment.py",
                                 description="Creates a slurm file that can be submitted on the hydra cluster",
                                 epilog="")

parser.add_argument("-f", "--file", type=str, default="test",
                 help="Relative filepath to the experiment file")
args = parser.parse_args()

filepath = args.file

def plotJSON(filepath):
    f = open(PATH + filepath)
    setup = json.load(f)
    f.close()

    experimentName = setup["experiment"]
    nNodes = setup["nNodes"]
    perNode = setup["perNode"]
    mpilibs = setup["mpilib"]
    plotFolder = setup["plotFolder"]
    procSetup = str(nNodes) + "x" + str(perNode)

    def fetchDataFromCluster():
        # check for data locally
        isLocal = True
        for mpilib in mpilibs:
            isLocal = os.path.exists(PATH + "../data/{0}/{1}x{2}{3}.txt".format(experimentName,
                                                                                nNodes,
                                                                                perNode,
                                                                                mpilib))
            if not isLocal:
                break

        if isLocal:
            ans = ""
            while ans not in ["y", "n", "Y", "N"]:
                ans = input("There is local data fetched. Do you want to use local data? (y/n)")
            if ans in ["n", "N"]:
                isLocal = False

        if not isLocal:
            # retrieve data
            print("Processing experiment " + experimentName)
            print("Fetching data from cluster...")
            result = subprocess.run(
                "scp -r hydra:~/data/{0} {1}../data".format(experimentName, PATH),
                shell=True, capture_output=True, text=True
            )
            print("...done")


    def plotData():
        # parse data
        graphs = setup[setup["plotGraphs"]]
        xAxistype = setup["xAxis"]
        xAxis = setup[xAxistype]


        # sorting by graph
        data = {g: {x: [] for x in xAxis} for g in graphs}

        # data from different MPI libs are saved in different files 
        if setup["plotGraphs"] == "mpilib":
            for g in graphs:
                f = open(PATH + "../data/{0}/{1}x{2}{3}.txt".format(experimentName,
                                                                                nNodes,
                                                                                perNode,
                                                                                g))
                line = f.readline()
                while line != "":
                    chunks = line.split(",")
                    mean = float (chunks[6].split(":")[1])
                    if xAxistype == "setSize":
                        xval = int (chunks[3].split(":")[1])            
                    if xAxistype == "filling":
                        xval = int (chunks[4].split(":")[1])
                    data[g][xval].append(mean)
                    line = f.readline()
                f.close()

        # process and plot data
        fig, ax = plt.subplots()
        for g in graphs:
            # calculate error and mean for every x entry
            y = []
            yerr = []
            for x in xAxis:
                vals = np.array(data[g][x])
                std = np.std(vals)
                nVals = np.shape(vals)[0]
                y.append(np.mean(vals))
                yerr.append(2*std/np.sqrt(nVals))
            ax.errorbar(xAxis, y, yerr=yerr, ls="--", label=g, capsize=5)

        if not setup["plotSize"]=="":
            width, height = plotsizes[setup["plotSize"]]
            fig.set_figwidth(width)
            fig.set_figheight(height)    
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.grid(True, which='major', linewidth=1.5)
        ax.grid(True, which='minor', linewidth=0.5, linestyle='--')
        plt.xlabel(xAxistype)
        plt.ylabel("Execution time [s]")
        plt.legend()
        plt.tight_layout()
        if not os.path.exists(PATH + "../plots/" + plotFolder):
            os.makedirs(PATH + "../plots/" + plotFolder)

        plt.savefig(PATH + "../plots/" + plotFolder + "/" + procSetup + experimentName + ".png", dpi=DPI)


    fetchDataFromCluster()
    plotData()

if os.path.isfile(filepath):
    plotJSON(filepath)
elif os.path.isdir(filepath):
    for root, _, files in os.walk(filepath):
        for file in files:
            plotJSON(os.path.join(root, file))
else:
    print(f"Error: '{filepath}' is not a valid file or directory.", file=sys.stderr)
    sys.exit(1)
