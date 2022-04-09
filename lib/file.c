#include <stdio.h>
#include "file.h"

void print_file(char *data, char *user_data) {

    File *file = (File *) data;
    printf("[%s] - Size: %ld\n", file -> path_file, file -> bytes_size);

}

void print_files(GList *file_list) {

    g_list_foreach(file_list, (GFunc) print_file, NULL);

}