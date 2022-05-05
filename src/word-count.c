#include <stdio.h>
#include "glib.h"
#include "mpi.h"
#include "input.h"
#include "file.h"
#include "log.h"

#define MASTER 0

#define TAG_NUM_FILES 100
#define TAG_SPLITTING 101
#define TAG_MERGE 102

MPI_Datatype create_file_type();

int main (int argc, char **argv) {

    int rank;                       // Id of processor
    int size;                       // Number of processors

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Init logger: create log dir and file for each proces
    init_logger();
    // Use this function for enable/disable logging
    set_logger(1);

    GList *file_list;               // List of files to read
    off_t total_bytes;              // Total of bytes to read
    int n_files;                    // Number of files to send/recv

    // Check and read input 
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

            // Read all file in input and skip file with error
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

        // Print status of file list
        print_files(file_list);

    }

    MPI_Datatype file_type;         // File MPI Datatype
    File *files;                    // Files to send/recv
    int slaves_size;                // Number of slaves
    int position;                   // Index for pack/unpack message

    // Create MPI datatype for File type
    file_type = create_file_type();
    // Get number of slaves
    slaves_size = size - 1;

    if (rank == MASTER) {

        // Calculate number of bytes to send for each processes 
        off_t bytes_for_each_processes = total_bytes / slaves_size;
        off_t rest = total_bytes % slaves_size;

        // Get number of files
        n_files = g_list_length(file_list);
        // Create files array to send
        files = malloc((sizeof *files) * n_files);

        // Create pointer for read the list
        GList *iterator_file_list = file_list;

        // Index for current_offset offset
        long int current_offset = 0;

        // We declare a buffer matrix, i.e. a buffer for each slave
        char buffer[BUFSIZ][slaves_size];
        // Create request array for sends
        MPI_Request requests[slaves_size];

        // Divide file for each process
        for (int i_slave = 0; i_slave < slaves_size; i_slave++) {

            // Calculate number of bytes to send
            off_t total_bytes_to_send = bytes_for_each_processes;
            if (rest > 0) {
                total_bytes_to_send++;
                rest--;
            }

            // Files index
            int i_file = 0;

            // Populates an array of files with all the necessary files 
            // to send to the processor "the slaves"
            while (total_bytes_to_send > 0) {

                // Get file and size
                File *file = (File *) (iterator_file_list -> data);
                off_t bytes = (file -> bytes_size);
                
                // Calculate the remaining bytes for file
                off_t remaining_bytes = bytes - current_offset;

                // If we don't need to split file on more processes
                if (total_bytes_to_send >= remaining_bytes) {
        
                    // Sub bytes sended
                    total_bytes_to_send -= remaining_bytes;
                    // Set the offset to read the rest of the file
                    file -> start_offset = current_offset;
                    file -> end_offset = bytes; 
                    // Add file to files array
                    files[i_file++] = *file;
                    // Start reading next file from 0
                    current_offset = 0;
                
                } 

                // If we need to split file on more processes
                else {

                    // Init start and end offset
                    file -> start_offset = current_offset;
                    file -> end_offset = current_offset + total_bytes_to_send - 1;
                    // Add file to files array
                    files[i_file++] = *file;
                    // Assign current_offset to next unread position
                    current_offset += total_bytes_to_send;
                    // We leave the while to continue working on the same file
                    break;

                }

                // Get next file
                iterator_file_list = iterator_file_list -> next;

            }

            // Pack number of file and files for sending a single message
            position = 0;
            MPI_Pack(&i_file, 1, MPI_INT, &buffer[0][i_slave], BUFSIZ, &position, MPI_COMM_WORLD);
            MPI_Pack(files, i_file, file_type, &buffer[0][i_slave], BUFSIZ, &position, MPI_COMM_WORLD);

            // Send message
            MPI_Irsend(&buffer[0][i_slave], BUFSIZ, MPI_PACKED, i_slave + 1, TAG_NUM_FILES, 
                MPI_COMM_WORLD, &requests[i_slave]);

        }

        // Free list of files and array of files to send
        free(files);
        g_list_free(file_list);

    } else {

        // Declare buffer for recv message
        char buffer[BUFSIZ];

        // Get number of files and files to read
        MPI_Status status;
        MPI_Recv(buffer, BUFSIZ, MPI_PACKED, MASTER, TAG_NUM_FILES, MPI_COMM_WORLD, &status);

        // Unpack buffer
        position = 0;
        MPI_Unpack(buffer, BUFSIZ, &position, &n_files, 1, MPI_INT, MPI_COMM_WORLD);
        files = malloc((sizeof *files) *n_files);
        MPI_Unpack(buffer, BUFSIZ, &position, files, n_files, file_type, MPI_COMM_WORLD);

        // Print my reading portion
        print_splitting(rank, n_files, files);

    }

    // Free file type
    MPI_Type_free(&file_type);

    // Create slave communicator
    MPI_Comm slaves_comm;
    MPI_Comm_split(MPI_COMM_WORLD, (rank == 0), rank, &slaves_comm);

    if (rank == MASTER) {
        
        int remaining_slaves = slaves_size;

        for (int i = 1; remaining_slaves > 0; remaining_slaves /= 2, i *= 2) {

            if ((remaining_slaves % 2) == 1) {

                int last_slave_rank = 1 + (remaining_slaves - 1) * i;
                print_communication(LOG_RECV, MASTER, last_slave_rank, TAG_MERGE);

            }

        }


    } else {

        // Create hash map for counting word inside files
        GHashTable *map_words = g_hash_table_new(g_str_hash, g_str_equal);

        // Count word for each files recived
        for (int i = 0; i < n_files; i++) {
            File file = files[i];
            count_words(&map_words, file.path_file, file.start_offset, file.end_offset);
        }

        // Print local hash map
        print_map_word(rank, map_words);

        // Get slave rank
        int slave_rank;
        MPI_Comm_rank(slaves_comm, &slave_rank);

        for (int remaining_slaves = slaves_size, i = 1, j = 2; 
            remaining_slaves > 0; remaining_slaves /= 2, i *= 2, j *= 2) {

            // If there are no other slaves left, I send the data to the master
            if (remaining_slaves == 1) {
                print_communication(LOG_SEND, rank, MASTER, TAG_MERGE);
                break;
            }

            // I will recv data from the next i-process
            if ((slave_rank % j) == 0) {

                // If i don't have a match i will send my data to MASTER directly
                if ((rank + i) >= size || (rank + i * 2) >= size) {
                    print_communication(LOG_SEND, rank, MASTER, TAG_MERGE);
                    break;
                }

                print_communication(LOG_RECV, rank, rank + i, TAG_MERGE + i);

            }
            // I will send data to previous i-proces
            else {

                print_communication(LOG_SEND, rank, rank - i, TAG_MERGE + i);
                break;

            }

        }

        /*for (int i = 2, j = 1; size / i > 0; i *= 2, j *= 2) {

            // If there are only 2 slaves left
            double remaining_slaves = ((double) size - 1) / i;
            if (remaining_slaves <= 1.0) {
                print_communication(LOG_SEND, rank, MASTER, TAG_MERGE);
                break;                
            }

            // I will recv data from the next j process
            if ((slave_rank % i) == 0) {

                // If i don't have a match i will send my data to MASTER directly
                if ((rank + j) == size) {
                    print_communication(LOG_SEND, rank, MASTER, TAG_MERGE);
                    break;
                }

                print_communication(LOG_RECV, rank, rank + j, TAG_MERGE + i);

            } 

            // I will send data to previous j proces
            else {

                print_communication(LOG_SEND, rank, rank - j, TAG_MERGE + i);
                break;

            }

        }*/

    }
    
    MPI_Finalize();
    return EXIT_SUCCESS;

}

MPI_Datatype create_file_type() {

    MPI_Datatype file_type;
    MPI_Aint base_address;
    MPI_Aint displacements[4];
    int lengths[4] = { MAX_PATH_LEN, 1, 1, 1 };
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