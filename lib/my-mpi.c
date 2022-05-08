#include <stdio.h>
#include <stdlib.h>

#include "my-mpi.h"

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

MPI_Datatype create_word_type() {

    MPI_Datatype word_type;
    MPI_Aint base_address;
    MPI_Aint displacements[2];
    int lengths[2] = { MAX_WORD_LEN, 1};
    Word dummy_word;

    MPI_Get_address(&dummy_word, &base_address);
    MPI_Get_address(&dummy_word.lexeme, &displacements[0]);
    MPI_Get_address(&dummy_word.occurrences, &displacements[1]);
    
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);

    MPI_Datatype types[] = { MPI_CHAR, MPI_UNSIGNED };
    MPI_Type_create_struct(2, lengths, displacements, types, &word_type);
    MPI_Type_commit(&word_type);

    return word_type;

}

void send_files_to_slaves(int size, GList *file_list, off_t total_bytes, MPI_Datatype file_type) {

    // Get number of slaves
    int n_slaves = size - 1;

    // Calculate number of bytes to send for each processes 
    off_t bytes_for_each_processes = total_bytes / n_slaves;
    off_t rest = total_bytes % n_slaves;

    // Get number of files
    guint n_files = g_list_length(file_list);
    // Create buffer for send files
    File *files = malloc((sizeof *files) * n_files);

    // Create pointer for read the list
    GList *iterator_file_list = file_list;

    // Reading index of the current file
    long current_offset = 0;

    // We declare a buffers matrix, i.e. a buffer for each slave
    char **buffers = malloc((sizeof **buffers) * n_slaves);
    // Create request array for sends
    MPI_Request requests[n_slaves];

    // Divide file for each process
    for (int i_slave = 0; i_slave < n_slaves; i_slave++) {

        // Calculate number of bytes to send
        off_t total_bytes_to_send = bytes_for_each_processes;
        if (rest > 0) {
            total_bytes_to_send++;
            rest--;
        }

        // Files index
        int i_file = 0;

        // Populates an array of files with all the necessary files 
        // to send to the processor "i_slave"
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

        // Get number of files to send
        guint n_files = i_file;

        // Allocate memory for buffer
        *(buffers + i_slave) = malloc((sizeof *buffers) * BUFFER_SIZE);

        // Pack number of file and files for sending a single message
        int position = 0;    
        MPI_Pack(&n_files, 1, MPI_UNSIGNED, &buffers[0][i_slave], BUFFER_SIZE, &position, MPI_COMM_WORLD);
        MPI_Pack(files, i_file, file_type, &buffers[0][i_slave], BUFFER_SIZE, &position, MPI_COMM_WORLD);

        // Send n_files and files with single message
        MPI_Irsend(&buffers[0][i_slave], BUFFER_SIZE, MPI_PACKED, i_slave + 1, TAG_NUM_FILES, 
            MPI_COMM_WORLD, &requests[i_slave]);

    }

    // As soon as the send is finished, free up the buffer memory.
    for (int i = 0; i < n_slaves; i++) {
     
        int index;
        MPI_Waitany(n_slaves, requests, &index, MPI_STATUS_IGNORE);

        free(buffers[index]);

    }

    // Free list of files and array of buffer
    free(files);
    free(buffers);

}

void recv_files_from_master(guint *n_files, File **files, MPI_Datatype file_type) {

    // Allocate buffer for recv message
    char *buffer = malloc((sizeof *buffer) * BUFFER_SIZE);
    
    // Get number of files and files to read
    MPI_Recv(buffer, BUFFER_SIZE, MPI_PACKED, MASTER, TAG_NUM_FILES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Unpack buffer
    int position = 0;
    MPI_Unpack(buffer, BUFFER_SIZE, &position, n_files, 1, MPI_UNSIGNED, MPI_COMM_WORLD);
    *files = malloc((sizeof **files) * *n_files);
    MPI_Unpack(buffer, BUFFER_SIZE, &position, *files, *n_files, file_type, MPI_COMM_WORLD);

    // Free up memory for buffer
    free(buffer);

}

void send_words_to_master(GHashTable **hash_table, MPI_Datatype word_type) {

    // Extract keys as GList and get number of keys
    GList *keys = g_hash_table_get_keys(*hash_table);
    guint n_words = g_list_length(keys);

    // Create request array for size and struct array send
    MPI_Request *requests = malloc((sizeof *requests) * 2);
    
    // Send size to master with non-blocking send
    MPI_Isend(&n_words, 1, MPI_UNSIGNED, MASTER, TAG_MERGE_SIZE, MPI_COMM_WORLD, &requests[0]);

    // Convert HashMap in Word array and send it to MASTER
    Word *words = malloc(sizeof(*words) * n_words);

    // Create pointer for read the list
    GList *iterator_keys = keys;

    // Iterate keys list
    for (guint i = 0; i < n_words; i++) {

        // Get lexeme and occurrences
        char *lexeme =  (char *) (iterator_keys -> data);            
        unsigned int occurrences = GPOINTER_TO_UINT(g_hash_table_lookup(*hash_table, lexeme));
          
        // Create word struct
        Word *word = malloc(sizeof *word);
        strncpy(word -> lexeme, lexeme, MAX_WORD_LEN);
        word -> occurrences = occurrences;

        // Add to array and free
        words[i] = *word;
        free(word);

        // Get next word
        iterator_keys = iterator_keys -> next;

    }

    // Send words to master with non-blocking send
    MPI_Isend(words, n_words, word_type, MASTER, TAG_MERGE_STRUCT, MPI_COMM_WORLD, &requests[1]);

    // Waits for all requests to complete
    MPI_Waitall(2, requests, MPI_STATUS_IGNORE);

    // Free elements
    g_list_free(keys);
    free(requests);
    free(words);

}

void recv_words_from_slaves(int size, GHashTable **hash_table, MPI_Datatype word_type) {

    // Get number of slaves
    int n_slaves = size - 1;

    // Create array for the receptions related to the number of words for each slave
    MPI_Request *requests_merge_size = malloc((sizeof *requests_merge_size) * n_slaves);

    // Create array that will contains the number of words to be received for each slave
    guint *n_words_for_each_processes = malloc((sizeof *n_words_for_each_processes) * n_slaves);

    // Start receiving the number of words 
    for (int i_slave = 0; i_slave < n_slaves; i_slave++) {
        MPI_Irecv(&n_words_for_each_processes[i_slave], 1, MPI_UNSIGNED, i_slave + 1,
            TAG_MERGE_SIZE, MPI_COMM_WORLD, &requests_merge_size[i_slave]);
    }

    // Create array for the receptions related to the word list for each slave
    MPI_Request *requests_merge_struct = malloc((sizeof *requests_merge_struct) * n_slaves);

    // Create a buffer for each slave which will contain the words received
    Word **buffers = malloc((sizeof **buffers) * n_slaves);

    // Start receiving the word list as soon as we are informed of the number of words we will receive
    for (int i = 0; i < n_slaves; i++) {

        int index;
        MPI_Waitany(n_slaves, requests_merge_size, &index, MPI_STATUS_IGNORE);

        // Allocate buffer for recive words
        guint n_words = n_words_for_each_processes[index];
        *(buffers + index) = malloc((sizeof **buffers) * n_words);

        // Start receiving the word list
        MPI_Irecv(*(buffers + index), n_words, word_type, index + 1, 
            TAG_MERGE_STRUCT, MPI_COMM_WORLD, &requests_merge_struct[index]);

    }

    // Free requests related number of words
    free(requests_merge_size);
    
    // As soon as we receive the list of words from a processor, we populate the hashmap
    for (int i = 0; i < n_slaves; i++) {

        int index;
        MPI_Waitany(n_slaves, requests_merge_struct, &index, MPI_STATUS_IGNORE);

        // Get number of recived words from index-slave
        guint n_words = n_words_for_each_processes[index];

        // Add lexeme and occurrences on hash map
        for (int j = 0; j < n_words; j++) {

            // Get word from buffer
            Word word = buffers[index][j];
            
            // If hash table contains lexeme, update occurrences
            if (g_hash_table_contains(*hash_table, word.lexeme)) {
            
                char *lexeme = strdup(word.lexeme);
                unsigned int occurrences = GPOINTER_TO_UINT(g_hash_table_lookup(*hash_table, lexeme));
                occurrences += word.occurrences;
                g_hash_table_replace(*hash_table, lexeme, GUINT_TO_POINTER(occurrences));
            
            } 
            // Else just add lexeme and occurrences
            else {
            
                char *lexeme = strdup(word.lexeme);
                g_hash_table_insert(*hash_table, lexeme, GUINT_TO_POINTER(word.occurrences));
            
            }

        }

        // Free buffer of index-slave
        free(buffers[index]);

    }

    // Free elements
    free(requests_merge_struct);
    free(n_words_for_each_processes);
    free(buffers);

}