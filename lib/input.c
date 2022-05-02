#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "input.h"

void check_input(int argc, char **argv, int *operation) {

    if (argc < 3) {

        printf("Usage: mpirun -np [num_processors] ./word-count [-d] [path of directory]\n");
        printf("Usage: mpirun -np [num_processors] ./word-count [-f] [path of file(s)]\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    
    } else {
    
        if (strcmp(argv[1], "-d") == 0)
            *operation = FLAG_DIR;

        else if (strcmp(argv[1], "-f") == 0)
            *operation = FLAG_FILES;
        
        else {
            
            printf("[word-count]: unrecognized operation '%s'\n", argv[1]);
            printf("Usage: mpirun -np [num_processors] ./word-count [-d] [path of directory]\n");
            printf("Usage: mpirun -np [num_processors] ./word-count [-f] [path of file(s)]\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);

        }

    }

}