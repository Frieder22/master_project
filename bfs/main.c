#include "csr_custom.h"
#include "bfs_custom.h"
#include "vertexSet.h"
#include "mpi.h"
#include "testStructure.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int mpi_size, mpi_rank;
extern char *optarg;
extern int optind, opterr, optopt;
distributedGraph_CSR graph;
bfs_custom setup;

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    char* filepath;
    int opt;
    while((opt = getopt(argc, argv, ":if:lrx")) != -1) 
    { 
        switch(opt) 
        {  
            case 'f': 
                if (mpi_rank==0)  printf("filename: %s\n", optarg); 
                filepath = optarg;
                break; 
            case ':': 
                printf("option needs a value\n"); 
                break; 
            case '?': 
                printf("unknown option: %c\n", optopt);
                break; 
        } 
    }
    DistributedGraph_Init(&graph, filepath);
    setup.func = UNION_BUTTERFLY;
    setup.graph = &graph;
    make_graph_data_structure(&setup);
    uint32_t *preds = malloc(graph.nGlobalVerts*sizeof(uint32_t));
    clean_pred(preds);
    run_bfs(3, preds);

    free(preds);
    free_graph_data_structure();
    DistributedGraph_Deinit(&graph);
    
    MPI_Finalize();
    return 0;
}
