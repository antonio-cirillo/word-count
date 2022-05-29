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
    File files[n_slaves][n_files];

    // Create pointer for read the list
    GList *iterator_file_list = file_list;

    // Reading index of the current file
    long current_offset = 0;

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
                files[i_slave][i_file++] = *file;
                // Start reading next file from 0
                current_offset = 0;
                
            } 

            // If we need to split file on more processes
            else {

                // Init start and end offset
                file -> start_offset = current_offset;
                file -> end_offset = current_offset + total_bytes_to_send - 1;
                // Add file to files array
                files[i_slave][i_file++] = *file;
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

        // Send files to i-slave
        MPI_Isend(&files[i_slave][0], n_files, file_type, i_slave + 1, TAG_NUM_FILES,
            MPI_COMM_WORLD, &requests[i_slave]);

    }

    // Wait all send are completed
    MPI_Waitall(n_slaves, requests, MPI_STATUS_IGNORE);

}

void recv_files_from_master(guint *n_files, File **files, MPI_Datatype file_type) {

    // Get number of files to read
    MPI_Status status;
    MPI_Probe(MASTER, TAG_NUM_FILES, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, file_type, n_files);

    // Allocate buffer for recv message
    *files = malloc((sizeof **files) * *n_files);

    // Get files to read
    MPI_Recv(*files, *n_files, file_type, MASTER, TAG_NUM_FILES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

}

void send_words_to_master(GHashTable *hash_table, MPI_Datatype word_type) {

    // Create GHashTableIter for reading hash_table
    GHashTableIter hash_table_iter;
    g_hash_table_iter_init(&hash_table_iter, hash_table);

    // Get number of different words
    guint n_words = g_hash_table_size(hash_table);

    // If there isn't words to send
    if (0 == n_words) {
        MPI_Send(&n_words, 1, MPI_UNSIGNED, MASTER, TAG_MERGE, MPI_COMM_WORLD);
        return ;
    }

    // Convert HashMap in Word array and send it to MASTER
    Word words[n_words];

    // Iterate HashTable
    gpointer key, value;
    for (guint i = 0; g_hash_table_iter_next(&hash_table_iter, &key, &value); i++) {
        
        // Get lexeme and occurrences
        char *lexeme = key;
        unsigned int occurrences = GPOINTER_TO_UINT(value);

        // Create word struct
        Word word;
        strncpy(word.lexeme, lexeme, MAX_WORD_LEN);
        word.occurrences = occurrences;

        // Add to array
        words[i] = word;

    }

    // Send words to master with blocking send
    MPI_Send(words, n_words, word_type, MASTER, TAG_MERGE, MPI_COMM_WORLD);

}

void recv_words_from_slaves(int size, GHashTable **hash_table, MPI_Datatype word_type) {

    // Get number of slaves
    int n_slaves = size - 1;

    // Start receiving the word list
    for (int i_slave = 0; i_slave < n_slaves; i_slave++) {

        // Wait for any slave to send its histogram
        MPI_Status status;
        MPI_Probe(MPI_ANY_SOURCE, TAG_MERGE, MPI_COMM_WORLD, &status);

        // Get source and number of word that source will send to master
        int n_words;
        MPI_Get_count(&status, word_type, &n_words);
        int source = status.MPI_SOURCE;

        // If there isn't word to recive, go next
        if (n_words < 0)
            continue;

        // Get words from source
        Word words[n_words];
        MPI_Recv(words, n_words, word_type, source, TAG_MERGE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        // Add lexeme and occurrences on hash map
        for (int i = 0; i < n_words; i++) {

            // Get word from buffer
            Word word = words[i];
            
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

    }

}