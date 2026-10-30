#include "bfs_custom.h"
#include "bitmap_custom.h"
#include "vertexSet.h"
#include "vertexSetIterator.h"

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>


int64_t visited_size;
unsigned long long *visited;
Vertexset *frontierOld, *frontierNew, *temp;
bool shift_bfs;


distributedGraph_CSR* graph_bfs;
extern int mpi_size, mpi_rank;
int func_bfs;
extern int bfs_iterations;


void make_graph_data_structure(bfs_custom* setup) {
	graph_bfs = setup->graph;
	func_bfs = setup->func;
	uint32_t nverts = graph_bfs->nGlobalVerts; 

	// init bitmap
	visited_size = (nverts + ulong_bits - 1) / ulong_bits;
	visited = malloc(visited_size*sizeof(unsigned long long));
		
	shift_bfs = (setup->func == UNION_SHIFT) || (setup->func == ALLREDUCE_SHIFT);
	frontierNew = (Vertexset*) malloc(sizeof(Vertexset));
	frontierOld = (Vertexset*) malloc(sizeof(Vertexset));
	
	Vertexset_Init(frontierOld, nverts, MPI_COMM_WORLD, shift_bfs);
	Vertexset_Init(frontierNew, nverts, MPI_COMM_WORLD, shift_bfs);
	
	
}

void printPreds(int32_t *preds){
	if (mpi_rank==0) {
		printf("preds:\n");
		for (size_t i = 0; i < graph_bfs->nGlobalVerts; i++) {
			printf("%d ,", preds[i]);
		}
		printf("\n");
	}
	
}

void Bitmap_UnionWithVertexset(unsigned long long* bitmap, Vertexset* vs){
	if (vs->isdense) {
		// dense variant
		for (size_t i = 0; i < vs->size_bitarray; i++) {
			bitmap[i] |= vs->bitArray[i];
		}
	} else {
		// sparse variant
		uint32_t vert;
		for (size_t i = 0; i < vs->sizeSparse; i++) {
			vert = vs->sparseArray[i];
			if (shift_bfs) {
				Bitmap_Set_Shifted(bitmap, vert, vs->indexShift, vs->size_bitarray);
			} else {
				Bitmap_Set(bitmap, vert);
			}
		}
	}
}

bool Vertexset_isFilled(Vertexset* vs){
	return vs->isdense + vs->sizeSparse;
}

void run_bfs_Union(int32_t root, int32_t* pred) {
	// get values for shifted bitmap
	int bitmapShift = frontierNew->indexShift;
	assert(visited_size == frontierNew->size_bitarray);

	// clean data structures
	Bitmap_Clean(visited, visited_size);
	Vertexset_Clean(frontierOld);
	Vertexset_Clean(frontierNew);
	
	// set in root
	Vertexset_Add(frontierOld, root);
	pred[root] = root;

	vertexSetIterator it;
	uint32_t vert, neigh;
	int iteration = 0;
	while (Vertexset_isFilled(frontierOld)) {
		Bitmap_UnionWithVertexset(visited, frontierOld);
		vertexSetIterator_Init(&it, frontierOld);
		while (vertexSetIterator_Has_next(&it)) {
			vert = vertexSetIterator_Next(&it);			
			for (uint64_t i = NEIGHSTART((graph_bfs), vert); i < NEIGHEND((graph_bfs),vert); i++) {
				neigh = graph_bfs->data[i];
				if (shift_bfs) {
				if (!Bitmap_Test_Shifted(visited, neigh, bitmapShift, visited_size)) {
					Bitmap_Set_Shifted(visited, neigh, bitmapShift, visited_size);
						Vertexset_Add(frontierNew, neigh);
						pred[neigh] = vert;
					}
				} else {
					if (!Bitmap_Test(visited, neigh)) {
						Bitmap_Set(visited, neigh);
						Vertexset_Add(frontierNew, neigh);
						pred[neigh] = vert;
					}
				}
			}
		}

		if (shift_bfs) {
			Vertexset_Union_Shift(frontierNew, VERTEXSET_OR);
		} else {
			Vertexset_Union_Butterfly(frontierNew, VERTEXSET_OR);
		}
		iteration++;

		//swap Vertexsets
		temp = frontierNew;
		frontierNew = frontierOld;
		frontierOld = temp;
		
		Vertexset_Clean(frontierNew);
	}
	bfs_iterations = iteration;
	
	// perform reduce over preds array
	MPI_Allreduce(MPI_IN_PLACE, pred, graph_bfs->nGlobalVerts, MPI_INT32_T, MPI_MAX, MPI_COMM_WORLD);	
}

bool Vertexset_isFilled_dense(Vertexset* vs){
	for (size_t i = 0; i < vs->size_bitarray; i++) {
		if (vs->bitArray[i] != 0UL) {
			return true;
		}
	}
	return false;
}

void run_bfs_Dense(int32_t root, int32_t* pred) {
	unsigned long long* bitmapTemp;
	int bitmapShift = frontierNew->indexShift;
	assert(visited_size == frontierNew->size_bitarray);

	// clean data structures
	Bitmap_Clean(visited, visited_size);
	Vertexset_Clean(frontierOld);
	Vertexset_Clean(frontierNew);
	frontierOld->isdense = true;

	// set in root
	Vertexset_Add(frontierOld, root);
	pred[root] = root;

	vertexSetIterator it;
	uint32_t vert, neigh;
	int iteration = 0;
	while (Vertexset_isFilled_dense(frontierOld)) {
		Vertexset_TransformToDense(frontierOld);
		Bitmap_UnionWithVertexset(visited, frontierOld);
		vertexSetIterator_Init(&it, frontierOld);
		while (vertexSetIterator_Has_next(&it)) {
			vert = vertexSetIterator_Next(&it);			
			for (uint64_t i = NEIGHSTART((graph_bfs), vert); i < NEIGHEND((graph_bfs),vert); i++) {
				neigh = graph_bfs->data[i];
				if (shift_bfs){
					if (!Bitmap_Test_Shifted(visited, neigh, frontierNew->indexShift, visited_size)) {
						Bitmap_Set_Shifted(visited, neigh, frontierNew->indexShift, visited_size);
						Vertexset_Add(frontierNew, neigh);
						pred[neigh] = vert;
					}
				} else {
					if (!Bitmap_Test(visited, neigh)) {
						Bitmap_Set(visited, neigh);
						Vertexset_Add(frontierNew, neigh);
						pred[neigh] = vert;
					}
				}
			}
		}
		Vertexset_TransformToDense(frontierNew);

		if (func_bfs == ALLREDUCE_BUTTERFLY_REVERSE) {
			Vertexset_Allreduce_Butterfly_ReverseBits(frontierNew, VERTEXSET_OR);
		} else if (func_bfs == ALLREDUCE_SHIFT) {
			Vertexset_Allreduce_Shift(frontierNew, VERTEXSET_OR);
		} else if (func_bfs == MPI_ALLREDUCE) {
		// Do union operation via allreduce
			MPI_Allreduce(frontierNew->bitArray, frontierNew->bitBuffer, frontierNew->size_bitarray,
							MPI_UNSIGNED_LONG_LONG, MPI_BOR, MPI_COMM_WORLD);
			bitmapTemp = frontierNew->bitArray;
			frontierNew->bitArray = frontierNew->bitBuffer;
			frontierNew->bitBuffer = bitmapTemp;		
		} else {
			assert(false);
		}
		
		//swap Vertexsets
		temp = frontierNew;
		frontierNew = frontierOld;
		frontierOld = temp;
		
		// Do cleanup manually to avoid sparse format
		Bitmap_Clean(frontierNew->bitArray, frontierNew->size_bitarray);
		iteration++;
	}
	bfs_iterations = iteration;

	// perform reduce over preds array
	MPI_Allreduce(MPI_IN_PLACE, pred, graph_bfs->nGlobalVerts, MPI_INT32_T, MPI_MAX, MPI_COMM_WORLD);	
}

void clean_pred(int32_t* pred) {
	for(int64_t i=0;i<graph_bfs->nGlobalVerts;i++) {
		pred[i] = -1;
	}
}

void run_bfs(int32_t root, int32_t* pred) {
	if (func_bfs == UNION_BUTTERFLY || func_bfs == UNION_SHIFT){
		run_bfs_Union(root, pred);
	} else {
		run_bfs_Dense(root, pred);
	}
}

void free_graph_data_structure(void) {
	free(visited);

	// deinit VertexSets
	free(frontierNew);
	free(frontierOld);
	Vertexset_Deinit(frontierOld);
	Vertexset_Deinit(frontierNew);
}
