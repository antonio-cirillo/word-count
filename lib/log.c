#include <stdio.h>
#include "glib.h"
#include "file.h"
#include "log.h"

int LOGGER_ON = 0;

void print_file(char *data, char *user_data) {

    File *file = (File *) data;
    printf("[%s] - Size: %ld\n", file -> path_file, file -> bytes_size);

}

void print_map_entry(char *key, char *value, char *user_data) {

    printf("%s: %d\n", key, GPOINTER_TO_INT(value));

}

void print_map_word(int rank, GHashTable *map_words) {

    if (LOGGER_ON) {
        printf("Processor #%d\n", rank);
        printf("----------------------------------------------------------------\n");   
        printf("Total of different word(s): %d\n\n", g_hash_table_size(map_words));
        g_hash_table_foreach(map_words, (GHFunc) print_map_entry, NULL);
    }

}

void print_files(GList *file_list) {

    if (LOGGER_ON) {
        printf("Total of file(s): %d\n", g_list_length(file_list));
        printf("----------------------------------------------------------------\n");   
        g_list_foreach(file_list, (GFunc) print_file, NULL);
        printf("\n");
    }

}

void print_splitting(int rank, int n, File files[]) {

    if (LOGGER_ON) {

        printf("Processor #%d\n", rank);
        printf("----------------------------------------------------------------\n");
        for (int i = 0; i < n; i++) {
            printf("Path of file: \t\t%s\n", files[i].path_file);
            printf("Bytes size: \t\t%ld\n", files[i].bytes_size);
            printf("Start offset: \t\t%ld\n", files[i].start_offset);
            printf("End offset: \t\t%ld\n\n", files[i].end_offset);
        }

    }

}

void setLogger(int flag) {
    LOGGER_ON = flag;
}