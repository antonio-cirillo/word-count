#include <stdlib.h>
#include "glib.h"

#define MAX_PATH_LEN 1024
#define MAX_WORD_LEN 256

typedef struct file {

    char path_file[MAX_PATH_LEN];
    off_t bytes_size;

    long int start_offset;
    long int end_offset;

} File;

off_t bytes_inside_dir(char *, GList **);

off_t bytes_of_file(char *, GList **);

int count_words(GHashTable **, char *, int, int);

void free_files(GList **);