#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "file.h"
#include "log.h"

int LOGGER_ON = 0;

void init_logger() {

    // Create if doens't exist a log directory
    struct stat st;
    if (stat("./log", &st) == -1)
        mkdir("./log", 0777);

}

void set_logger(int flag) {

    // Enable/disable logger   
    LOGGER_ON = flag;

}

void print_file(char *data, char *user_data) {

    FILE *fp = (FILE *) user_data;
    File *file = (File *) data;
    fprintf(fp, "[%s] - Size: %ld\n", file -> path_file, file -> bytes_size);

}

void print_files(GList *file_list) {

    if (LOGGER_ON) {

        FILE *fp = fopen("./log/master.txt", "w+b");
        
        fprintf(fp, "Total of file(s): %d\n", g_list_length(file_list));
        fprintf(fp, "----------------------------------------------------------------\n");   
        g_list_foreach(file_list, (GFunc) print_file, fp);

        fclose(fp);
    
    }

}

void print_splitting(int rank, int n, File files[]) {

    if (LOGGER_ON) {

        char *path_file = malloc(sizeof(*path_file) * 18);
        sprintf(path_file, "./log/slave_%d.txt", rank);
        FILE *fp = fopen(path_file, "w+b");

        fprintf(fp, "Process #%d\n", rank);
        fprintf(fp, "----------------------------------------------------------------\n");
        for (int i = 0; i < n; i++) {
            fprintf(fp, "Path of file: \t\t%s\n", files[i].path_file);
            fprintf(fp, "Bytes size: \t\t%ld\n", files[i].bytes_size);
            fprintf(fp, "Start offset: \t\t%ld\n", files[i].start_offset);
            fprintf(fp, "End offset: \t\t%ld\n\n", files[i].end_offset);
        }

        fclose(fp);
        free(path_file);

    }

}

void print_map_entry(char *key, char *value, char *user_data) {

    FILE *fp = (FILE *) user_data;
    fprintf(fp, "%s: %d\n", key, GPOINTER_TO_INT(value));

}

void print_map_word(int rank, GHashTable *map_words) {

    if (LOGGER_ON) {

        char *path_file = malloc(sizeof(*path_file) * 18);
        sprintf(path_file, "./log/slave_%d.txt", rank);
        FILE *fp = fopen(path_file, "a");

        fprintf(fp, "Total of different word(s): %d\n", g_hash_table_size(map_words));
        fprintf(fp, "----------------------------------------------------------------\n");   
        g_hash_table_foreach(map_words, (GHFunc) print_map_entry, fp);
        fprintf(fp, "----------------------------------------------------------------\n\n");   

        fclose(fp);
        free(path_file);

    }

}

void print_communication(int communication, int rank_from, int rank_to, int tag) {

   if (LOGGER_ON) {

        char *path_file = malloc(sizeof(*path_file) * 18);
        sprintf(path_file, "./log/slave_%d.txt", rank_from);
        FILE *fp = fopen(path_file, "a");

        switch (communication) {

            case LOG_SEND:  fprintf(fp, "[Tag #%d] - I will send data to process %d\n", tag, rank_to);
                            break;

            case LOG_RECV:  fprintf(fp, "[Tag #%d] - I will recive data from process %d\n", tag, rank_to);
                            break;

        }
        
        fclose(fp);
        free(path_file);

   }

}