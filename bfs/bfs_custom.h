#if !defined(BFS_CUSTOM)
#define BFS_CUSTOM

#include "csr_custom.h"
#include "bitmap_custom.h"
#include "vertexSet.h"
#include "vertexSetIterator.h"
#include "testStructure.h"

typedef struct bfs_custom {
	int func;
	distributedGraph_CSR* graph;
} bfs_custom;


void make_graph_data_structure(bfs_custom* setup);

void printPreds(int32_t *preds);

void run_bfs(int32_t root, int32_t* pred);

void clean_pred(int32_t* pred);

void free_graph_data_structure();

#endif // BFS_CUSTOM