#include <stdio.h>
#include "glib.h"
#include "mpi.h"
#include "file.h"
#include "log.h"

#define MASTER 0
#define FLAG_DIR 1
#define FLAG_FILES 2

void check_input(int argc, char **argv, int *operation);

int main (int argc, char **argv) {

    int rank;                       // id of processor
    int size;                       // number of processors

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == MASTER) {

        // Check number of processors
        if (size < 2) {
            printf("[word-count]: you have to run the program with more than one processor\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        // Check input and opeartion mode
        int operation;
        check_input(argc, argv, &operation);

        GList *file_list = NULL;
        off_t total_bytes = 0;

        // Create file list and calculate total of bytes        
        if (operation == FLAG_DIR) {
            
            total_bytes = bytes_inside_dir(argv[2], &file_list);

            // Check if there is a problem
            if (total_bytes == -1)
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE); 

            // Check if directory is empty
            if (total_bytes == 0) {
                printf("[word-count]: %s is empty\n", argv[2]);
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }

        } else {

            for (int i = 2; i < argc; i++) {
                total_bytes += bytes_of_file(argv[i], &file_list);
            }

        }

        printf("Total of file(s): %d\n", g_list_length(file_list));
        print_files(file_list);
        printf("---------------------------------------------------\n");   
        printf("Total of bytes: %ld\n", total_bytes);

    }

    MPI_Finalize();
    return EXIT_SUCCESS;

}

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