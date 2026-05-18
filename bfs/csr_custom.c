#include "csr_custom.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>
#include <stdio.h>

extern int mpi_rank, mpi_size;

bool useOriginalTG = false;
MPI_Datatype packed_edge_mpi_type;


static inline uint32_t max(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}

void initEdgeType(){
	int B[] = {1,1};
	MPI_Aint D[]={
		offsetof(edge, v0),
		offsetof(edge, v1)
	};
	MPI_Datatype T[] = {
		MPI_UINT32_T,
		MPI_UINT32_T
	};
	MPI_Type_create_struct(2, B, D, T, &packed_edge_mpi_type);
	MPI_Type_commit(&packed_edge_mpi_type);
}

void TupleGraph_distribute(tuple_graph* tg){
	// distribute general info about graph
	MPI_Bcast(&tg->nverts, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
	MPI_Bcast(&tg->nglobaledges, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);

	// calculate how many edges each rank gets
	assert(tg->nglobaledges < INT_MAX);
	int displ[mpi_size];
	int counts[mpi_size];
	displ[0] = 0;
	for (uint64_t i = 1; i < mpi_size + 1; i++) {
		displ[i] = i * tg->nglobaledges / mpi_size;
		counts[i-1] = displ[i] -  displ[i-1];
	}
	int nlocaledges = counts[mpi_rank];
	
	// allocate buffer
	edge* edgebuff = malloc(nlocaledges * sizeof(edge));

	// distribute edges
	MPI_Scatterv(tg->edgememory, counts, displ, packed_edge_mpi_type,
				edgebuff, nlocaledges, packed_edge_mpi_type,
				0, MPI_COMM_WORLD);
	
	// free roots original array
	if (mpi_rank==0) {
		free(tg->edgememory);
	}
	
	// set pointer
	tg->edgememory = edgebuff;
	tg->nlocaledges = nlocaledges;
	tg->distributed = true;
}

uint64_t* getVertexSpacing(const tuple_graph* const tg){
	uint32_t nverts = tg->nverts;
	uint64_t* vertexCount;
	vertexCount = (uint64_t*) malloc((nverts+1) * sizeof(uint64_t));
	
	// initialize with 0
	for (uint64_t i = 0; i < nverts+1; i++) {
		vertexCount[i] = 0;
	}

	// count occurence of each vertex
	edge edge;	
	for (uint64_t i = 0; i < tg->nlocaledges; i++) {
		edge = tg->edgememory[i];
		vertexCount[edge.v0+1]++;
		vertexCount[edge.v1+1]++;
	}

	// perform scan to get starting positions in CSR
	for (uint64_t i = 1; i < nverts + 1; i++) {
		vertexCount[i] += vertexCount[i-1];
	}

	assert(vertexCount[nverts] == 2*tg->nlocaledges && "Error in counting the edges"); 
	return vertexCount;
}

uint32_t* setDataArray(const tuple_graph* const tg, uint64_t* indices){
	uint32_t nverts = tg->nverts;
	uint32_t* vertexCount;
	uint32_t* data;
	uint32_t vertexOffset, vertex1, vertex2;
	edge edge;

	vertexCount = (uint32_t*) malloc(nverts * sizeof(uint32_t));
	data = (uint32_t*) malloc(indices[nverts] * sizeof(uint32_t));

	// initialize with 0
	for (uint32_t i = 0; i < nverts; i++) {
		vertexCount[i] = 0;
	}

	// fill in data
	for (uint64_t i = 0; i < tg->nlocaledges; i++) {
		// find corresponding edge
		edge = tg->edgememory[i];

		// set data in for outgoing direction
		vertex1 = edge.v0;
		vertex2 = edge.v1;
		vertexOffset = indices[vertex1];
		data[vertexCount[vertex1] + vertexOffset] = vertex2;
		vertexCount[vertex1]++;

		// set data in for ingoing direction
		vertexOffset = indices[vertex2];
		data[vertexCount[vertex2] + vertexOffset] = vertex1;
		vertexCount[vertex2]++;
	}
	
	free(vertexCount);
	return data;
}

void TupleGraph_Init(tuple_graph* tg, char* filepath){
	// rank 0 is master for IO
	if (mpi_rank == 0){
		// find number of lines (number of edges)
		FILE *fp = fopen(filepath, "r");
		if (!fp) printf("No file %s\n", filepath);
		
		uint64_t edgeCount = 0;
		int ch;
		
		while ((ch = getc(fp)) != EOF) {
			if (ch == '\n') edgeCount++;
		}
		
		
		// allocate memory
		tg->edgememory = malloc(edgeCount * sizeof(edge));
		
		// fill edgememory
		rewind(fp);
		int warning;
		uint32_t maxvert=0;
		for (size_t i = 0; i < edgeCount; i++) {
			warning = fscanf(fp, "%u %u", &tg->edgememory[i].v0, &tg->edgememory[i].v1);
			maxvert = max(maxvert, tg->edgememory[i].v0);
			maxvert = max(maxvert, tg->edgememory[i].v1);
		}
		if (warning !=2 ) {
			printf("File '%d' could not be read correctly\n", warning);
		}

		// set other info
		tg->nglobaledges = edgeCount;
		tg->nverts = maxvert + 1;
		fclose(fp);
	}
	// info for all ranks
	tg->nlocaledges = 0;		
	tg->distributed = false;
}

void TupleGraph_Deinit(tuple_graph* tg){
	if(tg->distributed){
		// all ranks have memory allocated
		free(tg->edgememory);
	} else {
		// only master has memory allocated
		if (mpi_rank == 0)   free(tg->edgememory);
	}
}

void DistributedGraph_Init(distributedGraph_CSR* const graph, char *filepath){
	initEdgeType();

	tuple_graph tg;
	TupleGraph_Init(&tg, filepath);
	TupleGraph_distribute(&tg);

    // set graph basic data
	graph->nGlobalEdges = tg.nglobaledges;
	graph->nGlobalVerts = tg.nverts;
	graph->nLocaledges = tg.nlocaledges;
    
	// calculate indices
	graph->indices = getVertexSpacing(&tg);

	// put in data
	graph->data = setDataArray(&tg, graph->indices);

	// Deinit tuple graph
	TupleGraph_Deinit(&tg);
}

void DistributedGraph_Deinit(distributedGraph_CSR* const graph){
	free(graph->data);
	free(graph->indices);
	MPI_Type_free(&packed_edge_mpi_type);
}