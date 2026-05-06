import json
import subprocess
import os
import sys
import matplotlib.pylab as plt
import numpy as np
import argparse
from matplotlib.ticker import LogLocator, LogFormatter, LogFormatterMathtext

PATH = ""
DPI = 300

ALLGATHER_TYPE = 0
ALLREDUCE_TYPE = 1
VERTEXSET_TYPE = 2

funcTypesToNumbers = {"Vertexset": VERTEXSET_TYPE,
                      "vertexset": VERTEXSET_TYPE,
                      "Allreduce": ALLREDUCE_TYPE,
                      "allreduce": ALLREDUCE_TYPE,
                      "Allgather": ALLGATHER_TYPE,
                      "allgather": ALLGATHER_TYPE}

CRITSIZE_STRAT_REV = {0: "max",
                  1: "min",
                  2: "mean"
}

FOUNDATION_ALLREDUCE = {"UNION_BUTTERFLY": "ALLREDUCE_BUTTERFLY_REVERSE",
                        "UNION_MOSTLY_HALFING": "ALLREDUCE_SHIFT" 
}

FOUNDATION_ALLREDUCE = {"UNION_BUTTERFLY": "ALLGATHER_BUTTERFLY_REVERSE",
                        "UNION_MOSTLY_HALFING": "ALLGATHER_SHIFT" 
}


plotColors = {
    "ALLREDUCE_BUTTERFLY":          '#66CCEE',  # Cyan
    "ALLREDUCE_BUTTERFLY_REVERSE":  '#AA3377',  # Purple
    "ALLREDUCE_SHIFT":              '#000000',  # Black
    "ALLGATHER_BUTTERFLY":          '#4477AA',  # Blue
    "ALLGATHER_BUTTERFLY_REVERSE":  '#EE6677',  # Red
    "ALLGATHER_SHIFT":              '#228833',  # Green
    "reference":                    "#D51414",
    "Reference":                    "#D51414"   
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


def plotRefAllgather(setup, ax):    
    f = open(PATH + "../data/{0}/{1}x{2}{3}.txt".format(setup["refAllgathData"],
                                                        setup["nNodes"],
                                                        setup["perNode"],
                                                        setup["mpilib"]))
    line = f.readline()

    dataAllgather = {}
    while line != "":
        chunks = line.split(",")
        mean = float (chunks[6].split(":")[1])                
        filling = int (chunks[4].split(":")[1]) 
        if filling in dataAllgather:
            dataAllgather[filling].append(mean)
        else:
            dataAllgather[filling] = [mean]
        line = f.readline()
    f.close()

    x_Allgather = []
    for x in dataAllgather:
        x_Allgather.append(x)

    x_Allgather.sort()
    y=[]
    yerr=[]
    for x in x_Allgather:
        vals = np.array(dataAllgather[x])
        std = np.std(vals)
        nVals = np.shape(vals)[0]
        y.append(np.mean(vals))
        yerr.append(2*std/np.sqrt(nVals))
    ax.errorbar(x_Allgather, y, yerr=yerr, ls="--", label="Reference Allgather", color="black", capsize=5)

def plotRefAllreduce(setup, ax, xAxis):
    refAllredVals = []
    f = open(PATH + "../data/{0}/{1}x{2}{3}.txt".format(setup["refAllredData"],
                                                        setup["nNodes"],
                                                        setup["perNode"],
                                                        setup["mpilib"]))
    line = f.readline()
    while line != "":
        chunks = line.split(",")
        mean = float (chunks[6].split(":")[1])
        setSize = int (chunks[3].split(":")[1])            
        if setSize == setup["setSize"]:
            refAllredVals.append(mean)
        line = f.readline()
    f.close()
    refAllred = np.mean(np.array(refAllredVals))
    ax.hlines(refAllred, xmin=0.5*xAxis[0], xmax=2*xAxis[-1], colors="black", label="Reference Allreduce")

def getData(setup, funcSetup):
    f = open(PATH + "../data/{0}/{1}x{2}{3}.txt".format(funcSetup["folder"],
                                                        setup["nNodes"],
                                                        setup["perNode"],
                                                        funcSetup["mpilib"]))
    line = f.readline()
    xAxistype = setup["xAxis"]
    xVals = setup[xAxistype]
    xVals.sort()

    funcTypeNeeded = funcTypesToNumbers[funcSetup["funcType"]]
    vsFuncNeeded = 0
    if funcTypeNeeded == VERTEXSET_TYPE:
        vsFuncNeeded = testnumber[funcSetup["vsFunc"]]

    # parse data
    data = {x:[] for x in xVals}
    while line != "":
        chunks = line.split(",")
        funcType = int(chunks[1].split(":")[1])
        vsFunc = int(chunks[2].split(":")[1])
        mean = float (chunks[6].split(":")[1])
        if xAxistype == "setSize":
            xval = int (chunks[3].split(":")[1])            
        if xAxistype == "filling":
            xval = int (chunks[4].split(":")[1])
        if funcType == funcTypeNeeded and vsFunc == vsFuncNeeded:
            data[xval].append(mean)
        line = f.readline()
    f.close()

    # calculate mean and stderr
    y = []
    yerr = []
    for x in xVals:
        vals = np.array(data[x])
        std = np.std(vals)
        nVals = np.shape(vals)[0]
        y.append(np.mean(vals))
        yerr.append(2*std/np.sqrt(nVals))

    return xAxistype, np.array(xVals), np.array(y), np.array(yerr)

def finishPlot(ax, fig, xVals, xAxistype, plotFolder, setup):
    procSetup = str(setup["nNodes"]) + "x" + str(setup["perNode"])

    if "ylim" in setup:
        ax.set_ylim(*setup["ylim"])

    if not setup["plotSize"]=="":
        width, height = plotsizes[setup["plotSize"]]
        fig.set_figwidth(width)
        fig.set_figheight(height)
    ax.set_xlim(0.5*xVals[0], xVals[-1]*2)
    ax.set_xscale("log")
    ax.grid(True, which='major', linewidth=1.5)
    ax.grid(True, which='minor', linewidth=0.5, linestyle='--')
    plt.xlabel(xAxistype)
    plt.legend()
    plt.tight_layout()
    if not os.path.exists(PATH + "../plots/" + plotFolder):
        os.makedirs(PATH + "../plots/" + plotFolder)

    plt.savefig(PATH + "../plots/" + plotFolder + "/" + procSetup + plotFolder + ".png", dpi=DPI)


def plotJSONcompareUnscaled(setup, plotFolder):
    referenceSetup = setup["refFunc"]
    name = referenceSetup["name"]
    # begin plotting
    fig, ax = plt.subplots()

    xAxistype, xVals, means_ref, errs_ref = getData(setup, referenceSetup)
    ax.errorbar(xVals, means_ref, yerr=errs_ref, ls="--", label=name, capsize=5, color=plotColors[name])


    funcSetups = setup["funcs"]
    for funcSetup in funcSetups:
        name = funcSetup["name"]
        xAxistype, xVals, means_ref, errs_ref = getData(setup, funcSetup)
        ax.errorbar(xVals, means_ref, yerr=errs_ref, ls="--", label=name, capsize=5, color=plotColors[name])
    
    ax.set_yscale("log")
    plt.ylabel("Execution time [s]")

    # end plotting
    finishPlot(ax, fig, xVals, xAxistype, plotFolder, setup)

def plotJSONcompareScaled(setup, plotFolder):
    referenceSetup = setup["refFunc"]
    name = referenceSetup["name"]
    # begin plotting
    fig, ax = plt.subplots()

    xAxistype, xVals, means_ref, errs_ref = getData(setup, referenceSetup)

    funcSetups = setup["funcs"]
    for funcSetup in funcSetups:
        name = funcSetup["name"]
        xAxistype, xVals, means, errs = getData(setup, funcSetup)

        relMeans = means/means_ref
        relMeans_err = relMeans * np.sqrt((errs_ref/means_ref)**2 + (errs/means)**2)

        ax.errorbar(xVals, relMeans, relMeans_err, ls="--", label=name, capsize=5, color=plotColors[name])

    ax.set_yscale("log")
    plt.ylabel("Relative execution time") 
    ax.hlines(1, 0.5*xVals[0], xVals[-1]*2, colors=plotColors["reference"])

    # end plotting
    finishPlot(ax, fig, xVals, xAxistype, plotFolder, setup)


def plotJSONcompare(setup, plotFolder):
    scaled = True
    if "scaled" in setup and setup["scaled"] in ["False", "false"]:
        scaled = False
    if scaled:
        plotJSONcompareScaled(setup, plotFolder)
    else:
        plotJSONcompareUnscaled(setup, plotFolder)




def plotJSON(filepath):
    f = open(filepath)
    setup = json.load(f)
    f.close()

    plotFolder = filepath.split("/")[1]
    if "experimentType" in setup and setup["experimentType"] == "compare":
        plotJSONcompare(setup, plotFolder)
        return

    experimentName = filepath.split("/")[1]
    nNodes = setup["nNodes"]
    perNode = setup["perNode"]
    mpilibs = setup["mpilib"]
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
            print("Processing experiment " + setup["experiment"])
            print("Fetching data from cluster...")
            result = subprocess.run(
                "scp -r hydra:~/data/{0} {1}../data".format(experimentName, PATH),
                shell=True, capture_output=True, text=True
            )
            print("...done")
            print(result)


    def plotDataClassic():
        # parse data
        graphs = setup[setup["plotGraphs"]]
        xAxistype = setup["xAxis"]
        xAxis = setup[xAxistype]
        xAxis.sort()


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
                
        # data from different MPI libs are saved in different files 
        if setup["plotGraphs"] == "critsizeStrategies":
            mpilib = setup["mpilib"]
            f = open(PATH + "../data/{0}/{1}x{2}{3}.txt".format(experimentName,
                                                                            nNodes,
                                                                            perNode,
                                                                            mpilib))
            line = f.readline()
            while line != "":
                chunks = line.split(",")
                mean = float (chunks[6].split(":")[1])
                if xAxistype == "setSize":
                    xval = int (chunks[3].split(":")[1])            
                if xAxistype == "filling":
                    xval = int (chunks[4].split(":")[1])
                strat = int (chunks[8].split(":")[1])
                data[CRITSIZE_STRAT_REV[strat]][xval].append(mean)
                line = f.readline()
            f.close()

        # process and plot data
        fig, ax = plt.subplots()

        # plot reference Allreduce
        if setup["plotGraphs"] == "critsizeStrategies" and "refAllredData" in setup and setup["refAllredData"] != "":
            plotRefAllreduce(setup, ax, xAxis)
        # plot reference Allgather data 
        if setup["plotGraphs"] == "critsizeStrategies" and "refAllgathData" in setup and setup["refAllgathData"] != "":
            plotRefAllgather(setup, ax)

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
        ax.set_xlim(0.5*xAxis[0], 2*xAxis[-1])
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
    if "noPlot" in setup and setup["noPlot"] in ["True", "true"]:
        return

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
