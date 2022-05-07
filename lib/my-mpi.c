#include <stdlib.h>

#include "my-mpi.h"
#include "file.h"

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

void recv_words_from_slaves(GHashTable **hash_table, int slaves_size, MPI_Datatype word_type) {

    // Create array for the receptions related to the number of words for each slave
    MPI_Request *requests_merge_size = malloc((sizeof *requests_merge_size) * slaves_size);

    // Create array that will contains the number of words to be received for each slave
    guint *n_words_for_each_processes = malloc((sizeof *n_words_for_each_processes) * slaves_size);

    // Start receiving the number of words 
    for (int i_slave = 0; i_slave < slaves_size; i_slave++) {
        MPI_Irecv(&n_words_for_each_processes[i_slave], 1, MPI_UNSIGNED, i_slave + 1,
            TAG_MERGE_SIZE, MPI_COMM_WORLD, &requests_merge_size[i_slave]);
    }

    // Create array for the receptions related to the word list for each slave
    MPI_Request *requests_merge_struct = malloc((sizeof *requests_merge_struct) * slaves_size);

    // Create a buffer for each slave which will contain the words received
    Word **buffers = malloc((sizeof **buffers) * slaves_size);

    // Start receiving the word list as soon as we are informed of the number of words we will receive
    for (int i = 0; i < slaves_size; i++) {

        int index;
        MPI_Waitany(slaves_size, requests_merge_size, &index, MPI_STATUS_IGNORE);

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
    for (int i = 0; i < slaves_size; i++) {

        int index;
        MPI_Waitany(slaves_size, requests_merge_struct, &index, MPI_STATUS_IGNORE);

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