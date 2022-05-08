#include <stdlib.h>
#include "glib.h"

#define FLAG_DIR 1
#define FLAG_FILES 2

void check_logging(int, char **);

void check_input(int, char **, int, int *);

void read_files(int, char **, int, int, GList **, off_t *);