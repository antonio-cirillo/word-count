#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glib.h"
#include "mpi.h"

#include "log.h"
#include "input.h"
#include "file.h"
#include "sort.h"
#include "my-mpi.h"

int main (int argc, char **argv) {

    /* ==========================================
    ================= INIT PHASE ================ 
    ========================================== */

    int rank;                       // Id of processor
    int size;                       // Number of processors
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    check_logging(argc, argv);

    /* ==========================================
    ========= CHECK AND READ INPUT PHASE ======== 
    ========================================== */

    GList *file_list = NULL;        // List of files to read
    off_t total_bytes = 0;          // Total of bytes to read

    if (rank == MASTER) {

        // Check input and get opeartion mode
        int operation;
        check_input(argc, argv, size, &operation);

        // Get list of files and total bytes
        read_files(argc, argv, size, operation, &file_list, &total_bytes);

        // Log files information
        print_files(file_list);

    }

    /* ==========================================
    ============= SPLIT FILES PHASE ============= 
    ========================================== */
    
    File *files;                    // List of files recived
    guint n_files;                  // Number of files recived

    // Create MPI datatype for File type
    MPI_Datatype file_type = create_file_type();

    if (rank == MASTER) {

        // Send files to slaves
        send_files_to_slaves(size, file_list, total_bytes, file_type);

        // Free list of files
        g_list_free(file_list);

    } else {

        // Recv files from master
        recv_files_from_master(&n_files, &files, file_type);

        // Print my reading portion
        print_splitting(rank, n_files, files);

    }

    // Free file type
    MPI_Type_free(&file_type);

    /* ==========================================
    ================= MAP PHASE ================= 
    ========================================== */

    GHashTable *hash_table;         // Hash map for map lexeme/occurrences

    if (rank != MASTER) {

        // Create hash map for counting word inside files
        hash_table = g_hash_table_new(g_str_hash, g_str_equal);

        // Count word for each files recived
        for (int i = 0; i < n_files; i++) {
            File file = files[i];
            count_words(&hash_table, file.path_file, file.start_offset, file.end_offset);
        }

        // Free files array
        free(files);

        // Print local hash map
        print_hash_table(rank, hash_table);

    }

    /* ==========================================
    ================ MERGE PHASE ================ 
    ========================================== */

    // Create word type
    MPI_Datatype word_type = create_word_type();

    if (rank == MASTER) {
        
        // Create a hashmap to merge words received from slaves
        hash_table = g_hash_table_new(g_str_hash, g_str_equal);

        // Recive words from slaves
        recv_words_from_slaves(size, &hash_table, word_type);

        // Print local hash map
        print_hash_table(rank, hash_table);

    } else {

        // Send words to master
        send_words_to_master(hash_table, word_type);

        // Free hash map
        g_hash_table_destroy(hash_table);

    }

    // Free word type
    MPI_Type_free(&word_type);

    /* ==========================================
    ================= SORT PHASE ================ 
    ========================================== */

    Word *words;
    guint n_words;

    if (rank == MASTER) {

        // Create GHashTableIter for reading hash_table
        GHashTableIter hash_table_iter;
        g_hash_table_iter_init(&hash_table_iter, hash_table);

        // Get number of different words
        n_words = g_hash_table_size(hash_table);

        // Allocate memory for words array
        words = malloc((sizeof *words) * n_words);
        
        // Iterate HashTable
        gpointer key, value;
        for (guint i = 0; g_hash_table_iter_next(&hash_table_iter, &key, &value); i++) {

            // Get lexeme and occurrences
            char *lexeme = key;
            unsigned int occurrences = GPOINTER_TO_UINT(value);

            // Create word struct
            Word *word = malloc(sizeof *word);
            strncpy(word -> lexeme, lexeme, MAX_WORD_LEN);
            word -> occurrences = occurrences;

            // Add to array and free
            words[i] = *word;
            free(word);

        }

        // Free hash map
        g_hash_table_destroy(hash_table);

        // Sort word with quick_sort
        quick_sort(words, 0, n_words - 1);

    }

    /* ==========================================
    ============== CREATE CSV PHASE ============= 
    ========================================== */
    
    if (rank == MASTER) {

        // Create file csv
        create_csv(n_words, words);

        // Free words array
        free(words);

    }

    MPI_Finalize();
    return EXIT_SUCCESS;

}