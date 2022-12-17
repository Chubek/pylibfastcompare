#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include <pthread.h>

#define K 4

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c\n"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


#define ALPHA_SIZE 4
#define CHAR_TO_IND(c) (((c == 'A') ? 0 : (c == 'C' ? 1 : (c == 'G' ? 2 : (c == 'T' || c == 'U' ? 3 : -1)))));
#define BUFFER_MAX 6000
#define SIZE_CHARS 32
#define SIZE_OUT 4
#define SIZE_HAM 32

#define PYHASH_X 0x345678
#define PYHASH_REM2 0xfffffffb
#define PYHASH_REM1 0xffffffef

#define A 0x000123fdea49fedc
#define B 0x930233fdaa39ffdd
#define C 0x112309df9edf91df
#define HM_SHIFT 32
#define PRIME_8_ONE 101
#define PRIME_8_TWO 25

#define HASH_MAX 120000
#define SZ_MAX 256
#define THREAD_CHUNK 2000


#define ROUNDUP_32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#define ROUNDUP_16(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, ++(x))

typedef uint8_t chartype_t;
typedef uint64_t outtype_t;
typedef uint8_t hamtype_t;
typedef uint32_t tuphash_t;
typedef uint8_t* seq_t;
typedef uint32_t hmsize_t;


typedef struct OutStruct {
    seq_t out;
    size_t out_len;
} out_s;


typedef struct HashMapNode {
    seq_t seq_packed;
    struct HashMapNode **dupes;
    size_t size_dup;
    size_t out_len;
    size_t index_in_array;
    int is_dup;
} clusterseq_s;
typedef clusterseq_s* clusterseqarr_t;

typedef struct HashMapValue {
    clusterseqarr_t arr;
    hmsize_t len_seq;
    hmsize_t n;
    tuphash_t hash;
} cluster_s;
typedef cluster_s* clusterarr_t;

typedef struct HashMapSt {
    clusterarr_t vec_vec;
    tuphash_t n;
    tuphash_t next_round;
} hm_s;

typedef struct NonZeroClusters {
    clusterarr_t clusters;
    tuphash_t size;
} non_zero_clusters_s;


int get_hamming_integers(hamtype_t a[SIZE_HAM], hamtype_t b[SIZE_HAM]);
void *hamming_cluster_single(void *cluster_ptr);
void hamming_clusters_hm(clusterarr_t non_zero_clusters, tuphash_t size);
void iterate_and_mark_dups(clusterseq_s lead, int out[]);
void cluster_ham_and_mark(chartype_t **seqs, size_t num_seqs, int k, int out[]);
void insert_seq_in_hm(hm_s *self, chartype_t *seq, size_t index_in_array, int k);
non_zero_clusters_s filter_out_zero_clusters(clusterarr_t clusters, tuphash_t size);
hmsize_t hash_bits(uint64_t x);
tuphash_t hash_tuple_to_index(uint64_t x, hmsize_t len);
hmsize_t next_round_bits32(hmsize_t n);
tuphash_t next_round_bits16(tuphash_t n);
hm_s *cluster_seqs(chartype_t **seqs_in, size_t num_seqs, int k);
void insert_resize_dupe(clusterseq_s *self, clusterseq_s *dupe);
int hamming_hseq_pair(clusterseq_s a, clusterseq_s b);
hm_s *new_hashmap();
void init_hmv(hm_s *self, tuphash_t index, hmsize_t len_seq);
void resize_insert_hmn(clusterseqarr_t self, hmsize_t index, seq_t seq_packed, size_t out_len, size_t index_in_array);
void resize_insert_hmv(cluster_s *self, seq_t seq_packed, size_t out_len, size_t index_in_array);
void resize_hashmap(hm_s *self);
void free_hashmap_vec(cluster_s self);
void free_hashmap(hm_s *self);
void mark_out(clusterarr_t clusters_arr, tuphash_t size, int out[]);
void insert_into_hashmap(hm_s *self, uint64_t key, seq_t  seq_packed, size_t len_seq, size_t out_len, size_t index_in_array);
cluster_s get_hashmap_value(hm_s *self, uint64_t key, hmsize_t len_seq);

