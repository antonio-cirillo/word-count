#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h> 
#include <sys/stat.h>

#include "file.h"

#define IS_CHARACTER(ch) ( isalpha(ch) || isdigit(ch) )

off_t bytes_inside_dir(GList **file_list, char* path) {

    off_t bytes_size = 0;
    struct dirent *de;
    DIR *dr;
    
    // Check if path is readable
    if ((dr = opendir(path)) == NULL)
        return -1;

    while ((de = readdir(dr)) != NULL) {

        // If dirent point a directory
        if ((de -> d_type) == DT_DIR) {

            char *dir_name = de -> d_name;
            // Check if dirent doesn't point on parent or actual directory
            if (strcmp(dir_name, ".") != 0 && strcmp(dir_name, "..") != 0) {
                
                // Call bytes_inside_dir on sub directory
                int str_len = strlen(path) + strlen(dir_name) + 2;
                char *new_path = (char *) malloc(str_len * (sizeof *new_path));
                snprintf(new_path, str_len, "%s/%s", path, dir_name);
                 
                int bytes = bytes_inside_dir(file_list, new_path);
                if (bytes > 0)
                    bytes_size += bytes;

                free(new_path);
            
            }

        } else {

            // Get path of file
            int str_len = strlen(path) + strlen(de -> d_name) + 2;
            char *path_file = (char *) malloc(str_len * (sizeof *path_file));
            snprintf(path_file, str_len, "%s/%s", path, de -> d_name);

            // Get size and add file to list
            int bytes = bytes_of_file(file_list, path_file);
            if (bytes > 0)
                bytes_size += bytes;
            else
                free(path_file);

        }

    }

    // Free up memory
    closedir(dr);
    free(de);
    free(dr);

    return bytes_size;

}

off_t bytes_of_file(GList **file_list, char *path) {

    // Check if path is a file and is readable
    struct stat st;
    if (stat(path, &st) == -1 || !S_ISREG(st.st_mode))
        return -1;
   
    // Get bytes size of file
    off_t bytes_size = st.st_size;

    // Check if file is empty
    if (0 == bytes_size)
        return 0;

    // Append new file to list
    File *file = malloc(sizeof *file);
    strncpy(file -> path_file, path, MAX_PATH_LEN);
    file -> bytes_size = bytes_size;
    *file_list = g_list_append(*file_list, file);
    
    // Return size of file
    return bytes_size;

}

void add_word_to_hash_table(GHashTable **map_words, char word[]) {

    // If hash table contains word, update counter
    if (g_hash_table_contains(*map_words, word)) {
    
        char *key = strdup(word);
        unsigned int value = GPOINTER_TO_UINT(g_hash_table_lookup(*map_words, key));
        value++;
        g_hash_table_replace(*map_words, key, GUINT_TO_POINTER(value));
    
    } else {
    
        char *key = strdup(word);
        g_hash_table_insert(*map_words, key, GUINT_TO_POINTER(1));
    
    }

}

int count_words(GHashTable **map_words, char *path, long start_offset, long end_offset) {

    // Open file
    FILE *file = fopen(path, "r");

    // Check if there is a problem on opening the file
    if (file == NULL)
        return EXIT_FAILURE;
    
    long remaining_bytes = end_offset - start_offset + 1;
    char ch;

    // If read doens't start from the begining of file  
    if (start_offset != 0) {

        // Change position to between of start offset
        fseek(file, start_offset - 1, SEEK_SET);
        ch = fgetc(file);
        
        // Check if we start reading a truncate word
        if (IS_CHARACTER(ch)) {
            
            // Skip word
            while (remaining_bytes > 0) {
                ch = fgetc(file);
                if (!IS_CHARACTER(ch)) {
                    remaining_bytes--;
                    break;
                } else
                    remaining_bytes--;
            }

        }

    }

    // Skip space
    while (remaining_bytes > 0) {
        ch = fgetc(file);
        if (IS_CHARACTER(ch)) {
            fseek(file, -1, SEEK_CUR);
            break;
        } else
            remaining_bytes--;
    }

    // If there is nothing else to read
    if (remaining_bytes == 0)
        return EXIT_SUCCESS;

    // Prepare buffer for word
    char word[MAX_WORD_LEN];
    int i = 0;

    // Read word from start to end
    while (remaining_bytes > 0) {

        // Read character
        ch = fgetc(file);
        remaining_bytes--;

        // Add to buffer
        if (IS_CHARACTER(ch))
            word[i++] = ch;
        
        else {
            
            // Add string terminator
            word[i] = '\0';
            // Add word inside hash table
            add_word_to_hash_table(map_words, word);
            // Reset index
            i = 0;

            // Skip space
            while (remaining_bytes > 0) {
                ch = fgetc(file);
                if (IS_CHARACTER(ch)) {
                    fseek(file, -1, SEEK_CUR);
                    break;
                } else
                    remaining_bytes--;
            }

        }

    }

    // If we trucate a word
    if (i > 0) {

        // Read truncate word
        ch = fgetc(file);
        while (IS_CHARACTER(ch)) {
            word[i++] = ch;
            ch = fgetc(file);
        }

        // Add string terminator
        word[i] = '\0';
        // Add word inside hash table
        add_word_to_hash_table(map_words, word);

    }

    fclose(file);
    return EXIT_SUCCESS;

}