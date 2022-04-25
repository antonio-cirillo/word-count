#include <stdlib.h>
#include "glib.h"

typedef struct file {
    char *path_file;
    off_t bytes_size;
} File;

off_t bytes_inside_dir(char *, GList **);

off_t bytes_of_file(char *, GList **);