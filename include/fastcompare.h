#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

typedef uint8_t chartype_t;
typedef uint64_t outtype_t;
typedef uint8_t hamtype_t;
typedef uint16_t tuphash_t;
typedef uint8_t* seq_t;
typedef uint32_t hmsize_t;

#include "hashmap.h"
#include "queue.h"
#include "cluster.h"
#include "utils.h"

#define ALPHA_SIZE 4
#define CHAR_TO_IND(c) (((c == 'A') ? 0 : (c == 'C' ? 1 : (c == 'G' ? 2 : (c == 'T' || c == 'U' ? 3 : -1)))));
#define BUFFER_MAX 6000
#define SIZE_CHARS 32
#define SIZE_OUT 4
#define SIZE_HAM 32

void cluster_ham_and_mark(chartype_t *seqs, int *len_seqs, size_t num_seqs, int k, int out[]);
void hamming_clusters_hm(clusterarr_t non_zero_clusters, tuphash_t size);
void *hamming_cluster_single(void *cluster_ptr);
int get_hamming_integers(hamtype_t a[SIZE_HAM], hamtype_t b[SIZE_HAM]);
int hamming_hseq_pair(clusterseq_s a, clusterseq_s b);