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

plotColors = {
    "ALLREDUCE_BUTTERFLY":          '#66CCEE',  # Cyan
    "ALLREDUCE_BUTTERFLY_REVERSE":  '#AA3377',  # Purple
    "ALLREDUCE_SHIFT":              '#000000',  # Black
    "ALLGATHER_BUTTERFLY":          '#4477AA',  # Blue
    "ALLGATHER_BUTTERFLY_REVERSE":  '#EE6677',  # Red
    "ALLGATHER_SHIFT":              '#228833',  # Green

}

yellow=  '#CCBB44',  # Yellow


testnumber = {}
testnumberInv = {}

def loadTestNumber():
    # do some shananigans to find the correct enum values from "testStructure.h"
    with open(PATH + "../vertexSet/testStructure.h", "r") as f:
        line = f.readline()
        while line != "enum Testcase{\n":
            line = f.readline()
        count = 0
        line = f.readline()
        while line != "};\n":
            word = line.split(",")[0].replace(" ", "").replace("\n","")
            testnumber[word] = count
            testnumberInv[count] = word
            count += 1
            line = f.readline()


def plotJSON(filepath):
    f = open(filepath)
    setup = json.load(f)
    f.close()

    experimentName = setup["experiment"]
    nNodes = setup["nNodes"]
    perNode = setup["perNode"]
    mpilibs = setup["mpilib"]
    plotFolder = setup["plotFolder"]
    procSetup = str(nNodes) + "x" + str(perNode)

    if type(mpilibs) != list:
        mpilibs = [mpilibs]
    
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


    def plotDataClassic():
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

    def identifyFuncName(chunks, reference):
        funcType = int(chunks[1].split(":")[1])
        if not funcType == VERTEXSET_TYPE:
            return reference
        vsFunc = int(chunks[2].split(":")[1])
        return testnumberInv[vsFunc]
        


    def plotDataCompareNative():
        # parse data
        graphs = setup["vsFunc"]
        reference = setup["mpilib"]
        xAxistype = setup["xAxis"]
        xAxis = setup[xAxistype]

        # sorting by graph
        data = {g: {x: [] for x in xAxis} for g in graphs}
        data[reference] = {x: [] for x in xAxis}

        # read data
        f = open(PATH + "../data/{0}/{1}x{2}{3}.txt".format(experimentName,
                                                                        nNodes,
                                                                        perNode,
                                                                        mpilibs[0]))
        line = f.readline()
        while line != "":
            chunks = line.split(",")
            g = identifyFuncName(chunks, reference)
            mean = float (chunks[6].split(":")[1])
            if xAxistype == "setSize":
                xval = int (chunks[3].split(":")[1])            
            if xAxistype == "filling":
                xval = int (chunks[4].split(":")[1])
            data[g][xval].append(mean)
            line = f.readline()
        f.close()

        # process reference data
        y_ref = []
        yerr_ref = []
        for x in xAxis:
            vals = np.array(data[reference][x])
            std = np.std(vals)
            nVals = np.shape(vals)[0]
            y_ref.append(np.mean(vals))
            yerr_ref.append(2*std/np.sqrt(nVals))
        y_ref = np.array(y_ref)
        yerr_ref = np.array(yerr_ref)

        # plot stuff
        fig, ax = plt.subplots()

        #plot reference line
        ax.hlines(1, 0.5*xAxis[0], xAxis[-1]*2, colors="#D51414")

        # process and plot data
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
            y = np.array(y)
            yerr = np.array(yerr)

            # calculate relative speed
            speed = y/y_ref
            speed_err = speed * np.sqrt((yerr_ref/y_ref)**2 + (yerr/y)**2)


            ax.errorbar(xAxis, speed, yerr=speed_err, ls="--", label=g, capsize=5, color=plotColors[g])

        if "ylim" in setup:
            ax.set_ylim(*setup["ylim"])

        if not setup["plotSize"]=="":
            width, height = plotsizes[setup["plotSize"]]
            fig.set_figwidth(width)
            fig.set_figheight(height)
        ax.set_xlim(0.5*xAxis[0], xAxis[-1]*2)
        ax.set_xscale("log")
        ax.grid(True, which='major', linewidth=1.5)
        ax.grid(True, which='minor', linewidth=0.5, linestyle='--')
        plt.xlabel(xAxistype)
        plt.ylabel("Relative execution time")
        plt.legend()
        plt.tight_layout()
        if not os.path.exists(PATH + "../plots/" + plotFolder):
            os.makedirs(PATH + "../plots/" + plotFolder)

        plt.savefig(PATH + "../plots/" + plotFolder + "/" + procSetup + experimentName + ".png", dpi=DPI)


    fetchDataFromCluster()
    if setup["plotGraphs"] == "compareAgainstNative":
        plotDataCompareNative()
    else:
        plotDataClassic()


if __name__ == "__main__":
    plotsizes = {"small": (4,4),
                "medium": (6,4)}

    parser = argparse.ArgumentParser(prog="plotExperiment.py",
                                    description="Creates a slurm file that can be submitted on the hydra cluster",
                                    epilog="")

    parser.add_argument("-f", "--file", type=str, default="test",
                    help="Relative filepath to the experiment file")
    args = parser.parse_args()

    filepath = PATH + args.file

    loadTestNumber()

    if os.path.isfile(filepath):
        plotJSON(filepath)
    elif os.path.isdir(filepath):
        for root, _, files in os.walk(filepath):
            for file in files:
                plotJSON(os.path.join(root, file))
    else:
        print(f"Error: '{filepath}' is not a valid file or directory.", file=sys.stderr)
        sys.exit(1)
