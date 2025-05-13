#if !defined(VERTEXSET)
#define VERTEXSET

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <bitmap_custom.h>
#include <mpi.h>

typedef struct Vertexset{
    // General information
    size_t maxsize;
    size_t sizeCrit;
    bool isdense;
    
    // Data for Bitarray
    unsigned long long *bitArray;
    unsigned long long *bitBuffer;
    uint32_t size_bitarray;
    
    // Data for sparse array
    size_t sizeSparse;
    uint32_t *sparseArray;
    uint32_t *sparseBuffer;
    int *sizesAll;
    int *displ;

    // mpi information
    MPI_Comm MPI_COMM;
    int mpi_rank;
    int mpi_size;
} Vertexset;

void Vertexset_Init(Vertexset* vs, uint32_t maxsize, MPI_Comm MPI_COMM);

void Vertexset_Add(Vertexset* vs, uint32_t vertex);

bool Vertexset_Contains(Vertexset* vs, uint32_t vertex);

bool Vertexset_TransformToDense(Vertexset* vs);

bool Vertexset_TransformToSparse(Vertexset* vs);

void Vertexset_Allreduce(Vertexset* vs, int VERTEXSET_OPERATION);

void Vertexset_Allreduce_Pure(Vertexset* vs, int VERTEXSET_OPERATION);

void Vertexset_Allreduce_Dynamic(Vertexset* vs, int VERTEXXSET_OPERATION);

void Vertexset_PrintSet(Vertexset* vs);

void Vertexset_Deinit(Vertexset* vs);


#define VERTEXSET_OR 0
#define VERTEXSET_AND 1
#define VERTEXSET_XOR 2

#endif // VERTEXSET
