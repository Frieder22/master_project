#if !defined(CSR_CUSTOM_H)
#define CSR_CUSTOM_H
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
typedef struct {
    uint32_t v0;
    uint32_t v1;
} edge;

typedef struct {
    bool distributed;
    uint32_t nverts;
    uint64_t nglobaledges; 
    uint64_t nlocaledges;
    edge* edgememory;
} tuple_graph;


typedef struct {
    size_t nGlobalEdges;
    size_t nLocaledges;
    uint32_t nGlobalVerts;
    uint32_t* data;
    uint64_t* indices;
} distributedGraph_CSR;

/**
 * Creates a CSR array to have fast access the edges of a node. The edges are distributed 
 * evenly on the ranks. (edge oriented distribution to ranks and vertex oriented organization
 * in each rank)
 * @return distributedGraph_CSR* graph
 */
void DistributedGraph_Init(distributedGraph_CSR* const graph, char *filepath);

/**
 * Correct cleanup of distributed graph struct
 */
void DistributedGraph_Deinit(distributedGraph_CSR* const graph);

#define NEIGHSTART(graph, vertex) graph->indices[vertex]
#define NEIGHEND(graph, vertex) graph->indices[vertex + 1]


#endif // CSR_CUSTOM_H
