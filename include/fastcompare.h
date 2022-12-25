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
typedef uint8_t buckethash_t;

#include "hashmap.h"
#include "fifo.h"
#include "cluster.h"
#include "utils.h"
#include "../ext-include/async-sem.h"
#include "../ext-include/async.h"
#include "prosumer.h"
#include "../generated-include/generated-all-workers.h"


#define ALPHA_SIZE 4
#define CHAR_TO_IND(c) (((c == 'A') ? 0 : (c == 'C' ? 1 : (c == 'G' ? 2 : (c == 'T' || c == 'U' ? 3 : -1)))));
#define fifo_MAX 6000
#define SIZE_CHARS 32
#define SIZE_OUT 4
#define SIZE_HAM 32
#define MAX_TIMEOUT 5

void cluster_ham_and_mark(chartype_t **seqs, size_t num_seqs, int k, int out[]);
void hamming_clusters_hm(non_zero_cluster_s *non_zero_clusters, tuphash_t size);
void *hamming_cluster_single(void *cluster_ptr);
void *hamming_cluster_list(void *cluster_ptr);
int get_hamming_integers(hamtype_t a[SIZE_HAM], hamtype_t b[SIZE_HAM]);
int hamming_hseq_pair(clusterseq_s a, clusterseq_s b);
void *parallel_pairwise_comparator(void *num);