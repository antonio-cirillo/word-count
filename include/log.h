#include "glib.h"

#define LOG_SEND 1
#define LOG_RECV 2

void init_logger();

void set_logger(int);

void print_files(GList *);

void print_map_word(int, GHashTable *);

void print_splitting(int, int, File[]);

void print_communication(int, int, int, int);