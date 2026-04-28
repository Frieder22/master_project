#if !defined(TESTSTRUCTURE)
#define TESTSTRUCTURE

#include "vertexSet.h"

typedef void (*vsFunc) (Vertexset*, int);

enum Testcase{
    ITERATOR,
    EXACT_HALFING,
    APROXIMATE_HALFING,
    NAIVE,
    ALLREDUCE_BUTTERFLY,
    ALLREDUCE_BUTTERFLY_REVERSE,
    ALLREDUCE_SHIFT,
    VERTEXSET_TEST,
    ALLGATHER
};

vsFunc getVertexsetFunc(int funcname){
    switch (funcname) {
    case EXACT_HALFING:
        return Vertexset_Union_Butterfly;
    case APROXIMATE_HALFING:
        return Vertexset_Union_Shift;
    case NAIVE:
        return Vertexset_Allreduce_Naive;
    case ALLREDUCE_BUTTERFLY:
        return Vertexset_Allreduce_Butterfly;
    case ALLREDUCE_BUTTERFLY_REVERSE:
        return Vertexset_Allreduce_Butterfly_ReverseBits;
    case ALLREDUCE_SHIFT:
        return Vertexset_Allreduce_Shift;
    case ALLGATHER:
        return Vertexset_Allgather;
    default:
        exit(1);
    }
}

bool getCommPattern(int funcname){
    switch (funcname) {
    case EXACT_HALFING:
        return false;
    case APROXIMATE_HALFING:
        return true;
    case ALLGATHER:
        return true;
    case ALLREDUCE_BUTTERFLY:
        return false;
    case ALLREDUCE_BUTTERFLY_REVERSE:
        return false;
    case ALLREDUCE_SHIFT:
        return true;
    default:
        exit(1);
    }
}

#endif // TESTSTRUCTURE

