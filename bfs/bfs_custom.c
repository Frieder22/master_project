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

/*

bool validate(int64_t root, int32_t *preds){
	// Indicator, that everything is correct
	bool isCorrect  = true;
	
	// Split Vertices onto ranks
	int32_t start_vert = graph_bfs->nGlobalVerts * (mpi_rank) / mpi_size;
	int32_t end_vert = graph_bfs->nGlobalVerts * (mpi_rank + 1) / mpi_size;
	
	// Phase 1: Check for cycles in preds and count level
	size_t level[graph_bfs->nGlobalVerts];
	{
		size_t i;
		int64_t pred;
		for (size_t vert = start_vert; vert < end_vert; vert++) {
			// find predecessor
			pred = preds[vert];

			// ignore verts, that have no predecessor
			if (pred == -1) {
				level[vert] = 0;
				continue;
			}

			// set root
			if (vert == root) {
				level[vert] = 0;
				continue;
			}
			
			i=1;
			while (i<graph_bfs->nGlobalVerts) {
				// check if pred is root
				if (pred == root) {
					level[vert] = i;
					break;
				}
				
				// Check if there is no pred
				if (pred == -1) {
					level[vert] = 0;
					break;
				}

				pred = preds[pred];
				i++;
			}
			if (!(i<graph_bfs->nGlobalVerts)) {
				printf("VALIDATION ERROR: Rank %d found a circular path in BFS-tree!\n", mpi_rank);
				isCorrect = false;
				break;
			}		
		}
	}

	// get displacements and counts from other ranks for MPI Collective
	int displ_mpi[mpi_size];
	int count_mpi[mpi_size];
	for (size_t r = 0; r < mpi_size; r++) {
		displ_mpi[r] = graph_bfs->nGlobalVerts * r / mpi_size;
		count_mpi[r] = graph_bfs->nGlobalVerts * (r+1) / mpi_size - displ_mpi[r];
	}
	// Communcicate calculated level to other ranks
	MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, level, count_mpi, displ_mpi, MPI_LONG, MPI_COMM_WORLD);
	

	// Phase 2: check, if counts of each pred-tree edge differs by exxactly one
	int64_t in;
	for (size_t out = start_vert; out < end_vert; out++) {
		in = preds[out];

		// only for connected edges and ones, that are not root
		if (in != -1 && out!= root) {
			int difference = level[out] - level[in];
			if (difference != 1) {
				printf("VALIDATION ERROR: Rank %d reported a predecessor with level difference not to 1!\n", mpi_rank);
				printf("difference: %d\n", difference);
				isCorrect = false;
				break;
			}
		}	
	}

	// Phase 3: Check, if every graph edge differs at most by one
	uint32_t neigh;
	uint64_t startIdx, endIdx;
	size_t level_vert, level_neigh;
	for (uint32_t vert = 0; vert < graph_bfs->nGlobalVerts; vert++) {
		for (uint64_t i = NEIGHSTART((graph_bfs), vert); i < NEIGHEND((graph_bfs), vert); i++) {
			neigh = graph_bfs->data[i];
			level_vert = level[vert];
			level_neigh = level[neigh];
			// if level == 0, vertex is either root or not reachable by BFS
			int difference = level_vert - level_neigh;
			if (difference > 1 || difference < -1) {
				printf("VALIDATION ERROR: Rank %d reported an edge with level differing more than 1!\n", mpi_rank);
				printf("difference: %d\n", difference);
				printf("root %d, pred1 %d, pred2 %d, levelRoot %d\n", root, preds[vert], preds[neigh], level[root]);
				isCorrect = false;
				break;
			}
		}
	}
	
	// Phase 4: Check, if all nodes were reached in BFS
	// <=> No included vertex should have a neighbor with pred[neigh] == -1
	for (uint32_t vert = 0; vert < graph_bfs->nGlobalVerts; vert++) {
		if (preds[vert] != -1) {
			for (uint64_t i = NEIGHSTART((graph_bfs), vert); i < NEIGHEND((graph_bfs), vert); i++) {
				neigh = graph_bfs->data[i];
				assert(neigh < graph_bfs->nGlobalVerts);
				if (preds[neigh] == -1) {
					printf("VALIDATION ERROR: Rank %d found a vertex, that was missed by BFS!\n", mpi_rank);
					printf("root %d, vertex %d, neigh %d\n", root, vert, neigh);
					printf("pred[vert] %d, pred[neigh] %d\n", preds[vert], preds[neigh]);
					isCorrect = false;
					break;
				}				
			}
			if (!isCorrect) {
				break;
			}
		}		
	}
	
	// Phase 5: Check, if all edges from tree appear in original graph
	int64_t pred;
	Vertexset checked;
	Vertexset_Init(&checked, graph_bfs->nGlobalVerts, MPI_COMM_WORLD, false);
	Vertexset_TransformToDense(&checked);
	for (size_t vert = 0; vert < graph_bfs->nGlobalVerts; vert++) {
		pred = preds[vert];
		
		// ignore not connected or root node
		if (pred == -1 || vert == root) {
			Vertexset_Add(&checked, vert);
		} else {
			// Check if any of the neighbors is pred
			for (uint64_t i = NEIGHSTART((graph_bfs), vert); i < NEIGHEND((graph_bfs), vert); i++) {
				neigh = graph_bfs->data[i];
				if (neigh == pred) {
					Vertexset_Add(&checked, vert);
				}
			}
		}
	}
	// Reduce with results from other ranks
	Vertexset_Allreduce_Butterfly_ReverseBits(&checked, VERTEXSET_OR);
	// Check for missing entries
	for (size_t vert = start_vert; vert < end_vert; vert++) {
		if (!Vertexset_Contains(&checked, vert)) {
			printf("VALIDATION ERROR: Rank %d found a tree edge, that is not in original graph!\n", mpi_rank);
			isCorrect = false;
		}
	}
	Vertexset_Deinit(&checked);
	
	


	// Communicate status with other ranks
	MPI_Allreduce(MPI_IN_PLACE, &isCorrect, 1, MPI_C_BOOL, MPI_LAND, MPI_COMM_WORLD);
	assert(isCorrect);
	return isCorrect;
}

*/
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
