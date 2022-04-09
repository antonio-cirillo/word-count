#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <sys/stat.h>
#include "glib.h"
#include "mpi.h"
#include "file.h"

#define MASTER 0

off_t bytes_inside_dir(char *, GList **);

int main (int argc, char **argv) {

    int rank;               //
    int size;               //

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == MASTER) {

        GList *file_list = NULL;
        
        off_t total_bytes = bytes_inside_dir(argv[1], &file_list);
        printf("Total of file(s): %d\n", g_list_length(file_list));
        print_files(file_list);
        printf("---------------------------------------------------\n");   
        printf("Total of bytes: %ld\n", total_bytes);

    }

    MPI_Finalize();
    return EXIT_SUCCESS;

}

off_t bytes_inside_dir(char* path, GList **list) {

    off_t bytes_size = 0;
    struct dirent *de;
    DIR *dr = opendir(path);

    while ((de = readdir(dr)) != NULL) {

        // If dirent point a directory
        if ((de -> d_type) == DT_DIR) {

            char *dir_name = de -> d_name;
            // Check if dirent doesn't point on parent or actual directory
            if (dir_name[0] != '.') {
                // Call bytes_inside_dir on sub directory
                char *new_path = (char *) malloc(strlen(path) + strlen(dir_name) + 1);
                sprintf(new_path, "%s/%s", path, dir_name);
                bytes_size += bytes_inside_dir(new_path, list);
                // free(new_path);
            }

        } else {

            // Get path of file
            char *path_file = (char *) malloc(strlen(path) + strlen(de -> d_name) + 1);
            sprintf(path_file, "%s/%s", path, de -> d_name);

            // Get bytes size of file
            struct stat st;
            stat(path_file, &st);
            off_t size = st.st_size;
            bytes_size += size;

            File *file = malloc(sizeof *file);
            file -> path_file = path_file;
            file -> bytes_size = size;

            *list = g_list_append(*list, file);

        }

    }

    return bytes_size;

}