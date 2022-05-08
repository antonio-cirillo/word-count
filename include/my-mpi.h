#include "mpi.h"
#include "glib.h"

#include "file.h"

#define MASTER 0

#define TAG_NUM_FILES 100
#define TAG_SPLITTING 101
#define BUFFER_SIZE 8192

#define TAG_MERGE_SIZE 102
#define TAG_MERGE_STRUCT 103

// Functions for creating derived data types
MPI_Datatype create_file_type();
MPI_Datatype create_word_type();

// Functions for split phase
void send_files_to_slaves(int, GList *, off_t, MPI_Datatype);
void recv_files_from_master(guint *, File **, MPI_Datatype);

// Functions for merge phase
void send_words_to_master(GHashTable **, MPI_Datatype);
void recv_words_from_slaves(int, GHashTable **, MPI_Datatype);