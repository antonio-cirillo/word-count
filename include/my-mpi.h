#include "mpi.h"
#include "glib.h"

#define MASTER 0

#define TAG_NUM_FILES 100
#define TAG_SPLITTING 101
#define TAG_MERGE_SIZE 102
#define TAG_MERGE_STRUCT 103

// Functions for creating derived data types
MPI_Datatype create_file_type();
MPI_Datatype create_word_type();

// Functions for merge phase
void send_words_to_master(GHashTable **, MPI_Datatype);
void recv_words_from_slaves(GHashTable **, int, MPI_Datatype);