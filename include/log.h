#include "glib.h"

#include "file.h"

#define LOG_SEND 1
#define LOG_RECV 2

int is_logger_on();

void set_logger(int);

void print_files(GList *);

void print_hash_table(int, GHashTable *);

void print_splitting(int, int, File[]);

void print_communication(int, int, int, int);