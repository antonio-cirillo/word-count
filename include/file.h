#include <stdlib.h>
#include "glib.h"

#define MAX_PATH_LEN 256
#define MAX_WORD_LEN 128

#ifndef FILE_H

#define FILE_H

typedef struct file {

    char path_file[MAX_PATH_LEN];
    off_t bytes_size;
    long start_offset;
    long end_offset;

} File;

typedef struct word {

    char lexeme[MAX_WORD_LEN];
    unsigned int occurrences;

} Word;

#endif

off_t bytes_inside_dir(GList **, char *);

off_t bytes_of_file(GList **, char *);

int count_words(GHashTable **, char *, long, long);