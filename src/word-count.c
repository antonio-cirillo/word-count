#include <stdio.h>
#include "glib.h"
#include "mpi.h"
#include "file.h"
#include "log.h"

#define MASTER 0
#define FLAG_DIR 1
#define FLAG_FILES 2

#define TAG_NUM_FILES 100
#define TAG_SPLITTING 101

void check_input(int argc, char **argv, int *operation);
MPI_Datatype create_file_type();

int main (int argc, char **argv) {

    GList *file_list = NULL;        // list of files to read
    int rank;                       // id of processor
    int size;                       // number of processors
    off_t total_bytes = 0;          // total of bytes to read

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Use this function for enable/disable logging
    setLogger(1);

    if (rank == MASTER) {

        // Check number of processors
        if (size < 2) {
            printf("[word-count]: you have to run the program with more than one processor\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        // Check input and opeartion mode
        int operation;
        check_input(argc, argv, &operation);

        // Create file list and calculate total of bytes        
        if (operation == FLAG_DIR) {
            
            total_bytes = bytes_inside_dir(argv[2], &file_list);

            // Check if there is a problem
            if (total_bytes == -1) {
                printf("[word-count]: couldn't open directory %s\n", argv[2]);
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }

        } else {

            for (int i = 2; i < argc; i++) {
                int bytes = bytes_of_file(argv[i], &file_list);
                if (bytes == -1)
                    printf("[word-count]: couldn't open file %s\n", argv[i]);
                else if (bytes == 0)
                    printf("[word-count]: file %s is empty, skipped...\n", argv[i]);
                else
                    total_bytes += bytes;
            }

        }

        // Check if input is a collection of files empty
        if (total_bytes == 0) {
            printf("[word-count]: input is empty\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        print_files(file_list);

    }

    MPI_Datatype file_type = create_file_type();

    if (rank == MASTER) {

        // Get number of slaves
        int slaves = size - 1;

        // Calculate number of bytes to send for each processes 
        off_t bytes_for_each_processes = total_bytes / slaves;
        off_t rest = total_bytes % slaves;

        // Scheduling files
        guint total_files = g_list_length(file_list);
        guint files_for_each_processes[slaves];

        GList *iterator_file_list = file_list;
        
        // Index for start offset
        long int start = 0;

        for (int i_slave = 0; i_slave < slaves; i_slave++) {

            // Calculate number of bytes to send
            off_t total_bytes_to_send = bytes_for_each_processes;
            if (rest > 0) {
                total_bytes_to_send++;
                rest--;
            }

            // Decleare buffer to send
            File files[total_files];
            // Index for files array
            int i_file = 0;

            // Populates an array of files with all the necessary files 
            // to send to the processor "the slaves"
            while (total_bytes_to_send > 0) {

                // Get file and size
                File *file = (File *) (iterator_file_list -> data);
                off_t bytes = (file -> bytes_size);
                
                // Calculate the remaining bytes for file
                off_t remaining_bytes = bytes - start;

                // If we don't need to split file on more processes
                if (total_bytes_to_send >= remaining_bytes) {
        
                    // Sub bytes sended
                    total_bytes_to_send -= remaining_bytes;
                    // Init start and end offset
                    file -> start_offset = start;
                    file -> end_offset = bytes; 
                    // Add file to files array
                    files[i_file++] = *file;
                    // Init start offest to 0
                    start = 0;
                
                } 

                // If we need to split file on more processes
                else {

                    // Init start and end offset
                    file -> start_offset = start;
                    file -> end_offset = start + total_bytes_to_send - 1;
                    // Add file to files array
                    files[i_file++] = *file;
                    // Init start offset from the next unread position
                    start += total_bytes_to_send;
                    // We leave the while to continue working on the same file
                    break;

                }

                // Get next file
                iterator_file_list = iterator_file_list -> next;

            }

            // Send number of file to send on slave "i_slave"
            MPI_Send(&i_file, 1, MPI_INT, i_slave + 1, TAG_NUM_FILES, MPI_COMM_WORLD);
            // Send files on slave
            MPI_Send(files, i_file, file_type, i_slave + 1, TAG_SPLITTING, MPI_COMM_WORLD);

        }

        g_list_free(file_list);

    } else {

        // Get number of files to read
        MPI_Status status;
        int n_files;
        MPI_Recv(&n_files, 1, MPI_INT, MASTER, TAG_NUM_FILES, MPI_COMM_WORLD, &status);

        // Get files to read
        File files[n_files];
        MPI_Recv(files, n_files, file_type, MASTER, TAG_SPLITTING, MPI_COMM_WORLD, &status);

        print_splitting(rank, n_files, files);

        // Map words
        GHashTable *map_words = g_hash_table_new(g_str_hash, g_str_equal);

        for (int i = 0; i < n_files; i++) {

            File file = files[i];
            count_words(&map_words, file.path_file, file.start_offset, file.end_offset);

        }

        g_hash_table_foreach(map_words, (GHFunc) print_map_file, NULL);

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

MPI_Datatype create_file_type() {

    MPI_Datatype file_type;
    MPI_Aint base_address;
    MPI_Aint displacements[4];
    int lengths[4] = { 
        MAX_PATH_LEN, 
        1, 
        1, 
        1 
    };
    File dummy_file;

    MPI_Get_address(&dummy_file, &base_address);
    MPI_Get_address(&dummy_file.path_file, &displacements[0]);
    MPI_Get_address(&dummy_file.bytes_size, &displacements[1]);
    MPI_Get_address(&dummy_file.start_offset, &displacements[2]);
    MPI_Get_address(&dummy_file.end_offset, &displacements[3]);
    
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);
    displacements[2] = MPI_Aint_diff(displacements[2], base_address);
    displacements[3] = MPI_Aint_diff(displacements[3], base_address);

    MPI_Datatype types[] = { MPI_CHAR, MPI_LONG, MPI_LONG, MPI_LONG };
    MPI_Type_create_struct(4, lengths, displacements, types, &file_type);
    MPI_Type_commit(&file_type);

    return file_type;

}