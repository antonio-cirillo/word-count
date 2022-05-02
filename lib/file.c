#include <stdio.h>
#include <string.h>
#include <dirent.h> 
#include <sys/stat.h>
#include "file.h"

#define IS_TERMINATOR(ch) ( (ch < 33) ? 1 : 0 )

off_t bytes_inside_dir(char* path, GList **list) {

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
                char *new_path = (char *) malloc(strlen(path) + strlen(dir_name) + 1);
                sprintf(new_path, "%s/%s", path, dir_name);
                 
                int bytes = bytes_inside_dir(new_path, list);
                if (bytes > 0)
                    bytes_size += bytes;

                free(new_path);
            
            }

        } else {

            // Get path of file
            char *path_file = (char *) malloc(strlen(path) + strlen(de -> d_name) + 1);
            sprintf(path_file, "%s/%s", path, de -> d_name);

            // Get size and add file to list
            int bytes = bytes_of_file(path_file, list);
            if (bytes > 0)
                bytes_size += bytes;
            else
                free(path_file);

        }

    }

    free(de);
    free(dr);
    return bytes_size;

}

off_t bytes_of_file(char* path, GList **list) {

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
    strncpy(file -> path_file, path, strlen(path));
    file -> bytes_size = bytes_size;

    *list = g_list_append(*list, file);
    return bytes_size;

}

void add_word_to_hash_table(GHashTable **map_words, char word[]) {

    // If hash table contains word, update counter
    if (g_hash_table_contains(*map_words, word)) {
    
        char *key = strdup(word);
        int value = GPOINTER_TO_INT(g_hash_table_lookup(*map_words, key));
        value++;
        g_hash_table_replace(*map_words, key, GINT_TO_POINTER(value));
    
    } else {
    
        char *key = strdup(word);
        g_hash_table_insert(*map_words, key, GINT_TO_POINTER(1));
    
    }

}

int count_words(GHashTable **map_words, char *path, int start_offset, int end_offset) {

    // Open file
    FILE *file = fopen(path, "r");

    // Check if there is a problem on opening the file
    if (file == NULL)
        return EXIT_FAILURE;
    
    char ch;

    // If read doens't start from the begining of file  
    if (start_offset != 0) {

        // Change position to start offset
        fseek(file, start_offset, SEEK_SET);
        ch = fgetc(file);

        // If we start reading a character check if is begining of word
        if (!IS_TERMINATOR(ch) && ch != EOF) {
            
            fseek(file, start_offset - 1, SEEK_SET);
            ch = fgetc(file);

            // If we start reading a truncate word, skip it
            if (!IS_TERMINATOR(ch))
                while (!IS_TERMINATOR(fgetc(file))) ;

        }

    }

    // Prepare buffer for word
    char word[MAX_WORD_LEN];
    int i = 0;

    // Read word from start to end
    while (ftell(file) <= end_offset) {

        // Read character
        ch = fgetc(file);

        // Add to buffer
        if (!IS_TERMINATOR(ch))
            word[i++] = ch;

        else {

            // Add string terminator
            word[i] = '\0';
            // Add word inside hash table
            add_word_to_hash_table(map_words, word);
            // Reset index
            i = 0;

            // Skip space
            while (ftell(file) <= end_offset) {
                ch = fgetc(file);
                if (!IS_TERMINATOR(ch) || ch == EOF)
                    break;
            }
            
            // If we finish to read exit
            if (ftell(file) > end_offset || ch == EOF)
                break;

            // Retract if we don't finish to read file
            fseek(file, ftell(file) - 1, SEEK_SET);

        }

    }

    // If we trucate a word
    if (i > 0) {
        
        // Read truncate word
        ch = fgetc(file);
        while (!IS_TERMINATOR(ch) && ch != EOF) {
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