#include <stdio.h>
#include <string.h>
#include "mpi.h"

#include "log.h"
#include "input.h"

void check_input(int argc, char **argv, int size, int *operation) {

    // Check number of processors
    if (size < 2) {
        printf("[word-count]: you have to run the program with more than one process\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Check number of arguments
    if (argc < 3) {

        printf("Usage: mpirun -np [num_processors] ./word-count [-log]? [-d] [path of directory]\n");
        printf("Usage: mpirun -np [num_processors] ./word-count [-log]? [-f] [path of file(s)]\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    
    } else {
        
        // If log flag is set
        if (strcmp(argv[1], "-log") == 0) {

            if (strcmp(argv[2], "-d") == 0 && argc == 4)
                *operation = FLAG_DIR;

            else if (strcmp(argv[2], "-f") == 0 && argc >= 4)
                *operation = FLAG_FILES;

            else {
            
                printf("[word-count]: unrecognized operation '%s'\n", argv[2]);
                printf("Usage: mpirun -np [num_processors] ./word-count [-d] [path of directory]\n");
                printf("Usage: mpirun -np [num_processors] ./word-count [-f] [path of file(s)]\n");
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            
            }

            // Enable logger
            set_logger(1);

        }

        else if (strcmp(argv[1], "-d") == 0 && argc == 3)
            *operation = FLAG_DIR;

        else if (strcmp(argv[1], "-f") == 0 && argc >= 3)
            *operation = FLAG_FILES;
        
        else {
            
            printf("[word-count]: unrecognized operation '%s'\n", argv[1]);
            printf("Usage: mpirun -np [num_processors] ./word-count [-d] [path of directory]\n");
            printf("Usage: mpirun -np [num_processors] ./word-count [-f] [path of file(s)]\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);

        }

    }

}

void read_files(int argc, char **argv, int size, int operation, GList **file_list, off_t *total_bytes) {

    int logger_flag = is_logger_on();

    // Input are a directory
    if (operation == FLAG_DIR) {
            
        *total_bytes = bytes_inside_dir(file_list, argv[2 + logger_flag]);
        
        // Check if there is a problem
        if (*total_bytes == -1) {
            printf("[word-count]: couldn't open directory %s\n", argv[2 + logger_flag]);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

    } 
    
    // Input are list of files
    else {
        
        // Read all file in input and skip file with error
        for (int i = 2 + logger_flag; i < argc; i++) {
           
            int bytes = bytes_of_file(file_list, argv[i]);
            if (bytes == -1)
                printf("[word-count]: couldn't open file %s\n", argv[i]);
            else if (bytes == 0)
                printf("[word-count]: file %s is empty, skipped...\n", argv[i]);
            else
                *total_bytes += bytes;
        
        }

    }
    
    // Check if input is a collection of files empty
    if (*total_bytes == 0) {
        printf("[word-count]: input is empty\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    
    // Check if number of processes is more than total of bytes to read
    if (size > *total_bytes) {
        printf("[word-count]: number of processes is more than total of bytes to read\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

}