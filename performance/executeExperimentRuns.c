#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mpi.h"
#include "vertexSet.h"
#include "cJSON.h"
#include "testStructure.h"

#define ALLGATHER_TYPE 0
#define ALLREDUCE_TYPE 1
#define VERTEXSET_TYPE 2

enum fillingDistributions{
    UNIFORM,
    CONCENTRATED
};

int mpi_rank, mpi_size;
cJSON *root, *runs;

struct inputParams{
    char inputFile[1024];
    int nExperimentRuns;
} inputParams;

struct experimentRun{
    char name[1024];
    int funcType;
    int vsFunc;
    int setSize;
    int filling;
    int nRepetitions;
    int distribution;
    int critSizeStrategy;
} experimentRun;

struct measuredData{
    double mean;
    int nAccepted;
} measuredData;


void saveData(){
    if (mpi_rank==0) {
        printf("name: %s, functype: %d, vsFunc: %d, setsize: %d, filling: %d, nRep: %d, mean: %e, nAccepted: %d, critSizeStrat: %d\n",
               experimentRun.name,
               experimentRun.funcType,
               experimentRun.vsFunc,
               experimentRun.setSize,
               experimentRun.filling,
               experimentRun.nRepetitions,
               measuredData.mean,
               measuredData.nAccepted,
               experimentRun.critSizeStrategy);
    }
}

int comp_double_asc(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;

    if (da < db) return -1;
    if (da > db) return  1;
    return 0;
}

void processData(double* execTimes){
    double* execTimesRoot;
    execTimesRoot = (double*) malloc(experimentRun.nRepetitions * sizeof(double));
    
    // Find longest execution times for each process
    MPI_Reduce(execTimes, execTimesRoot, experimentRun.nRepetitions, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (mpi_rank==0) {
        double Q1, Q3, IRQ;
        double lowerBound, upperBound;
        double mean;
        size_t count;

        // sort array
        qsort(execTimesRoot, experimentRun.nRepetitions, sizeof(double), comp_double_asc);

        // find quartiles Q_1 and Q_3
        Q1 = execTimesRoot[experimentRun.nRepetitions/4];
        Q3 = execTimesRoot[experimentRun.nRepetitions*3/4];
        IRQ = Q3-Q1;

        // find lower and upper bounds
        lowerBound = Q1 - 1.5*IRQ;
        upperBound = Q3 + 1.5*IRQ;

        // find mean of prunded data
        count = 0;
        for (size_t i = 0; i < experimentRun.nRepetitions; i++) {
            if (execTimesRoot[i] >= lowerBound && execTimesRoot[i] <= upperBound){
                mean += execTimesRoot[i];
                count++;
            }
        }
        mean = mean/count;
        measuredData.mean = mean;
        measuredData.nAccepted = count;
    }
    free(execTimesRoot);
}

void fillVertexset(Vertexset* vs, int nElementsTotal){
    // fill uniformly
    if (experimentRun.distribution == UNIFORM){
        //add elements
        for (size_t element = 0; element < nElementsTotal; element++) {
            if (element%mpi_size ==  mpi_rank)
            Vertexset_Add(vs, element);
        }
        return;
    }

    // fill concentrated
    if (experimentRun.distribution == CONCENTRATED){
        if (mpi_rank == 0) {
            for (size_t element = 0; element < nElementsTotal; element++) {
                Vertexset_Add(vs, element);
            }
        }
        return;
    }

    // throw if some option is not defined
    if (mpi_rank==0)
    printf("Wrong option:     %d\n", experimentRun.distribution);
    fflush(stdout);
    assert(false && "Distribution type not implemented for Vertexset.");
}

void vertexsetRun(double* execTimes, vsFunc func){
    bool shiftPattern = getCommPattern(experimentRun.vsFunc);
    //init stuff
    Vertexset vs;
    size_t nElementsTotal = experimentRun.filling;
    
    
    
    // if VS allgather is tested, only make set size large enough to fit
    if (experimentRun.vsFunc == ALLGATHER_SHIFT 
        || experimentRun.vsFunc == ALLGATHER_BUTTERFLY 
        || experimentRun.vsFunc == ALLGATHER_BUTTERFLY_REVERSE)
    {
        uint32_t setSize = (nElementsTotal+1) * sizeof(uint32_t);
        Vertexset_Init(&vs, setSize, MPI_COMM_WORLD, shiftPattern);
        if(experimentRun.filling > vs.sizeCrit)
        Vertexset_SetCritsize(&vs, experimentRun.filling + 1);
    } else {
            
        Vertexset_Init(&vs, experimentRun.setSize, MPI_COMM_WORLD, shiftPattern);
        // set critsize strategy
        Vertexset_SetCritsizeStrategy(&vs, (critSizeStrategies) experimentRun.critSizeStrategy);
    }

        
    //benchmark
    double start, end;
    for (size_t it = 0; it < experimentRun.nRepetitions; it++) {
        fillVertexset(&vs, nElementsTotal);
        MPI_Barrier(MPI_COMM_WORLD);
        start = MPI_Wtime();
        func(&vs, VERTEXSET_OR);
        end = MPI_Wtime();
        execTimes[it] = end - start;

        // remove elements
        Vertexset_Clean(&vs);
    } 

    //deinit
    Vertexset_Deinit(&vs);
}

int fillAllgather(int nElementsTotal){
    // fill uniformly
    if (experimentRun.distribution == UNIFORM){
        int sizeSparse = 0;
        for (size_t i = 0; i < nElementsTotal; i++) {
            if (i%mpi_size == mpi_rank){
                //arraySparseIn[sizeSparse] = i;
                sizeSparse++;
            }
        }
        return sizeSparse;
    }
    
    // fill concentrated
    if (experimentRun.distribution == CONCENTRATED){
        if (mpi_rank == 0) {
            return nElementsTotal;
        } else {
            return 0;
        }  
    }

    // throw if some option is not defined
    if (mpi_rank==0)
    printf("Wrong option:     %d\n", experimentRun.distribution);
    fflush(stdout);
    assert(false && "Distribution type not implemented for Allgather.");
}

void allgatherRun(double* execTimes){
    uint32_t *arraySparseOut, *arraySparseIn;
    int sizeSparse, nElementsTotal;
    int sparseSizes[mpi_size], displ[mpi_size];
    
    //init sparse
    nElementsTotal = experimentRun.filling;
    arraySparseIn = (uint32_t*) malloc(nElementsTotal * sizeof(uint32_t));
    arraySparseOut = (uint32_t*) malloc(nElementsTotal * sizeof(uint32_t));

    //benchmark
    double start, end;
    for (size_t it = 0; it < experimentRun.nRepetitions; it++) {
        sizeSparse = fillAllgather(nElementsTotal);
        
        MPI_Barrier(MPI_COMM_WORLD);
        start = MPI_Wtime();

        //find sparse sizes of other ranks
        sparseSizes[mpi_rank] = sizeSparse;
        MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, sparseSizes, 1, MPI_INT, MPI_COMM_WORLD);
        displ[0] = 0;
        for (size_t j = 1; j < mpi_size; j++) {
            displ[j] = displ[j-1] + sparseSizes[j-1];
        }
        // actual Allgather
        MPI_Allgatherv(arraySparseIn, sizeSparse, MPI_UINT32_T, arraySparseOut, sparseSizes, displ, MPI_UINT32_T, MPI_COMM_WORLD);
        
        end = MPI_Wtime();
        execTimes[it] = end - start;
    }
    

    //deinit
    free(arraySparseOut);
    free(arraySparseIn);
}

void allreduceRun(double* execTimes){
    unsigned long long* bitmap;

    // init buffer
    size_t size_bitarray = (experimentRun.setSize + ulong_bits - 1) / ulong_bits;
    bitmap = (unsigned long long*) malloc(size_bitarray * sizeof(unsigned long long));

    //benchmark
    double start, end;
    for (size_t it = 0; it < experimentRun.nRepetitions; it++) {
        MPI_Barrier(MPI_COMM_WORLD);
        start = MPI_Wtime();
        MPI_Allreduce(MPI_IN_PLACE, bitmap, size_bitarray, MPI_UNSIGNED_LONG_LONG, MPI_BOR, MPI_COMM_WORLD);
        end = MPI_Wtime();
        execTimes[it] = end - start;
    }

    //deinit
    free(bitmap);
}

void executeExperimentRun(){
    double* execTimes;
    vsFunc func;
    execTimes = (double*) malloc(experimentRun.nRepetitions*sizeof(double));

    //setup
    switch (experimentRun.funcType) {
    case VERTEXSET_TYPE:
        func = getVertexsetFunc(experimentRun.vsFunc); 
        vertexsetRun(execTimes, func);
        break;
    case ALLGATHER_TYPE:
        allgatherRun(execTimes);
        break;
    case ALLREDUCE_TYPE:
        allreduceRun(execTimes);
        break;
    default:
        break;
    }

    processData(execTimes);

    free(execTimes);
}

void setExperimentRunSetup(size_t i){
    char funcTypeStr[1024];
    char distrStr[1024];
    cJSON* run = cJSON_GetArrayItem(runs, i);
    strcpy(experimentRun.name, cJSON_GetObjectItem(run, "experiment")->valuestring);

    // parse functype
    strcpy(funcTypeStr, cJSON_GetObjectItem(run, "funcType")->valuestring);
    if (strcmp(funcTypeStr, "Allgather") == 0) {
        experimentRun.funcType = ALLGATHER_TYPE;
    } else if (strcmp(funcTypeStr, "Allreduce") == 0) {
        experimentRun.funcType = ALLREDUCE_TYPE;
    } else if (strcmp(funcTypeStr, "Vertexset") == 0) {
        experimentRun.funcType = VERTEXSET_TYPE;
    } else {
        if (mpi_rank==0)
        printf("Wrong type:     %s\n", funcTypeStr);
        assert(false && "funcType not defined.");
    }

    // parse distribution
    strcpy(distrStr, cJSON_GetObjectItem(run, "distribution")->valuestring);
    if (strcmp(distrStr, "uniform") == 0) {
        experimentRun.distribution = UNIFORM;
    } else if (strcmp(distrStr, "concentrated") == 0) {
        experimentRun.distribution = CONCENTRATED;
    } else {
        if (mpi_rank==0)
        printf("Wrong type:     %s\n", distrStr);
        fflush(stdout);
        assert(false && "Distribution type not defined.");
    }
    experimentRun.setSize = cJSON_GetObjectItem(run, "setSize")->valueint;
    experimentRun.filling = cJSON_GetObjectItem(run, "filling")->valueint;  
    experimentRun.nRepetitions = cJSON_GetObjectItem(run, "nRepetitions")->valueint;
    if (experimentRun.funcType==VERTEXSET_TYPE) {
        experimentRun.vsFunc = cJSON_GetObjectItem(run, "vsFunc")->valueint; // enum from testStructure.h needs to be used
    }
    // set critical size strategy, if defined
    cJSON* critSize_item = cJSON_GetObjectItem(run, "critSizeStrat");
    if (critSize_item != NULL) {
        experimentRun.critSizeStrategy = critSize_item->valueint;
    } else {
        experimentRun.critSizeStrategy = 0; //Default is CRITSIZE_MAX strategy
    }    
}   

static char *read_file(const char *path) {
    // simply safe whole file in buffer
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *buf = malloc(len + 1);
    size_t readlen = fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

void parseInput(int argc, char* argv[]){
    // find json file
    assert(argc==2);
    strcat(inputParams.inputFile, argv[1]);

    // parse json file to JSON structs
    char* json_file = read_file(inputParams.inputFile);
    assert(json_file && "Can not open JSON file.");
    root = cJSON_Parse(json_file);
    runs = cJSON_GetObjectItem(root, "runs");

    // add number of runs
    inputParams.nExperimentRuns = cJSON_GetArraySize(runs);

    // not needed anymore
    free(json_file);
}


int main(int argc, char *argv[]){
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    parseInput(argc, argv);
    
    //execute experiment runs
    for (size_t i = 0; i < inputParams.nExperimentRuns; i++) {
        setExperimentRunSetup(i);
        executeExperimentRun();
        saveData();
    }
    

    // Deinit stuff
    cJSON_Delete(root); 
    MPI_Finalize();
    return 0;
}