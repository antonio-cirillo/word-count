#include <sys/stat.h>
#include "glib.h"

typedef struct file {
    char *path_file;
    off_t bytes_size;
} File;

void print_files(GList *);