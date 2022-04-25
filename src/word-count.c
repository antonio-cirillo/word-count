#include <stdio.h>
#include "glib.h"
#include "mpi.h"
#include "file.h"
#include "log.h"

#define MASTER 0
#define INPUT_DIR 1
#define INPUT_FILES 2

int main (int argc, char **argv) {

    // Check and prepare input
    int input;

    if (argc < 3) {

        printf("Usage: mpirun -np [num_processors] ./word-count [-d] [path of directory]\n");
        printf("Usage: mpirun -np [num_processors] ./word-count [-f] [path of file(s)]\n");
        return EXIT_FAILURE;
    
    } else {
    
        if (strcmp(argv[1], "-d") == 0)
            input = INPUT_DIR;

        else if (strcmp(argv[1], "-f") == 0)
            input = INPUT_FILES;
        
        else {
            
            printf("word-count: unrecognized operation '%s'\n", argv[1]);
            printf("Usage: mpirun -np [num_processors] ./word-count [-d] [path of directory]\n");
            printf("Usage: mpirun -np [num_processors] ./word-count [-f] [path of file(s)]\n");
            return EXIT_FAILURE;

        }

    }

    int rank;                       // id of processor
    int size;                       // number of processors

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Check number of processors
    if (size < 2) {

        printf("word-count: you have to run the program with more than one processor\n");
        return EXIT_FAILURE;

    }

    if (rank == MASTER) {

        GList *file_list = NULL;
        off_t total_bytes = 0;
        
        if (input == INPUT_DIR) {
            
            total_bytes = bytes_inside_dir(argv[2], &file_list);
            printf("word-count: %s is empty\n", argv[2]);
         
        } else {

            for (int i = 2; i < argc; i++)
                total_bytes += bytes_of_file(argv[i], &file_list);

        }

        printf("Total of file(s): %d\n", g_list_length(file_list));
        print_files(file_list);
        printf("---------------------------------------------------\n");   
        printf("Total of bytes: %ld\n", total_bytes);

    }

    MPI_Finalize();
    return EXIT_SUCCESS;

}