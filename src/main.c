#include "vertexSet.h"
#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
int main(int argc, char const *argv[]) {
    Vertexset vs;

    Vertexset_Init(&vs, 32);

    Vertexset_TransformToDense(&vs);

    Vertexset_Add(&vs, 31);
    Vertexset_Add(&vs, 31);
    Vertexset_Add(&vs, 31);
    Vertexset_Add(&vs, 29);

    Vertexset_TransformToSparse(&vs);

    if(Vertexset_Contains(&vs, 31)){
        printf(ANSI_COLOR_GREEN "Correct!" ANSI_COLOR_RESET "\n");
    } else {
        printf(ANSI_COLOR_RED "NOT correct!" ANSI_COLOR_RESET "\n");
    }
    if(Vertexset_Contains(&vs, 29)){
        printf(ANSI_COLOR_GREEN "Correct!" ANSI_COLOR_RESET "\n");
    } else {
        printf(ANSI_COLOR_RED "NOT correct!" ANSI_COLOR_RESET "\n");
    }

    return 0;
}



