#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include <threads.h>
#include <stdatomic.h>

#define ALPHA_SIZE 4
#define CHAR_TO_IND(c) (((c == 'A') ? 0 : (c == 'C' ? 1 : (c == 'G' ? 2 : (c == 'T' || c == 'U' ? 3 : -1)))));
#define BUFFER_MAX 6000
#define SIZE_CHARS 32
#define SIZE_OUT 4
#define SIZE_HAM 4

#define PYHASH_X 0x345678
#define PYHASH_REM2 0xfffffffb
#define PYHASH_REM1 0xffffffef

#define A 0x000123fdea49fedc
#define B 0x930233fdaa39ffdd
#define C 0x112309df9edf91df
#define HM_SHIFT 32

#define SZ_MAX 256



#define ROUNDUP_32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#define ROUNDUP_16(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, ++(x))

typedef uint8_t chartype_t;
typedef uint64_t outtype_t;
typedef uint64_t hamtype_t;
typedef uint16_t tuphash_t;
typedef uint64_t* seq_t;
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
} cluster_s;
typedef cluster_s* clusterarr_t;

typedef struct HashMapSt {
    clusterarr_t vec_vec;
    tuphash_t n;
    tuphash_t next_round;
} hm_s;

int get_hamming_integers(hamtype_t a[SIZE_HAM], hamtype_t b[SIZE_HAM]);
void get_hamming_cluster(hamtype_t *in_cluster, size_t len_rows, int *out);
void hamming_cluster_single(cluster_s *cluster);
void reduce_integer_or_op(outtype_t in, outtype_t *reducer);
void hamming_clusters_hm(hm_s *clustered);
void iterate_and_mark_dups(clusterseq_s lead, size_t out[]);
void encode_gatacca(chartype_t in[SIZE_CHARS], outtype_t out[SIZE_OUT]);
void cluster_ham_and_mark(char **seqs, size_t num_seqs, size_t out[]);
outtype_t pack_32_bytes_in_64_bits(chartype_t in[SIZE_CHARS]);
out_s pack_seq_into_64bit_integers(chartype_t *seq, size_t len_str);
void insert_seq_in_hm(hm_s *self, char *seq, size_t index_in_array);
void hamming_cluster_single(cluster_s *cluster);
hmsize_t hash_bits(uint64_t x);
tuphash_t hash_tuple_to_index(uint64_t x, hmsize_t len);
hmsize_t next_round_bits32(hmsize_t n);
tuphash_t next_round_bits16(tuphash_t n);
hm_s *cluster_seqs(char **seqs_in, size_t num_seqs);
void insert_resize_dupe(clusterseq_s *self, clusterseq_s *dupe);
int hamming_hseq_pair(clusterseq_s a, clusterseq_s b);
hm_s *new_hashmap();
void init_hmv(hm_s *self, hmsize_t index, hmsize_t len_seq);
void resize_insert_hmn(clusterseqarr_t self, hmsize_t index, seq_t seq_packed, size_t out_len, size_t index_in_array);
void resize_insert_hmv(cluster_s *self, seq_t seq_packed, size_t out_len, size_t index_in_array);
void resize_hashmap(hm_s *self);
void free_hashmap_vec(cluster_s self);
void free_hashmap(hm_s *self);
void insert_into_hashmap(hm_s *self, uint64_t key, seq_t  seq_packed, size_t len_seq, size_t out_len, size_t index_in_array);
cluster_s get_hashmap_value(hm_s *self, uint64_t key, hmsize_t len_seq);
