#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <sys/stat.h>
#include "file.h"

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
    file -> path_file = path;
    file -> bytes_size = bytes_size;

    *list = g_list_append(*list, file);

    return bytes_size;

}