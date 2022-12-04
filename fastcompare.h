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

typedef uint8_t chartype_t;
typedef uint64_t outtype_t;
typedef uint64_t hamtype_t;
typedef uint64_t* seq_t;
typedef uint32_t hmsize_t;

typedef struct OutStruct {
    seq_t out;
    size_t out_len;
} out_s;



int get_hamming_integers(hamtype_t a[SIZE_HAM], hamtype_t b[SIZE_HAM]);
void get_hamming_cluster(hamtype_t *in_cluster, size_t len_rows, int *out);
void reduce_integer_or_op(outtype_t in, outtype_t *reducer);
void encode_gatacca(chartype_t in[SIZE_CHARS], outtype_t out[SIZE_OUT]);
outtype_t pack_32_bytes_in_64_bits(chartype_t in[SIZE_CHARS]);
out_s pack_seq_into_64bit_integers(chartype_t *seq);



hmsize_t hash_bits(uint64_t x);
hmsize_t hash_tuple_to_index(uint64_t x, hmsize_t len);
hmsize_t next_round_bits(hmsize_t n);


typedef struct HashMapNode {
    seq_t seq;
    char *seq_str;
    hmsize_t len_seq;
    size_t len_out;
} hm_node;
typedef hm_node* hmn_t;

typedef struct HashMapValue {
    hmn_t arr;
    hmsize_t len_seq;
    hmsize_t n;
} hmvalue_s;
typedef hmvalue_s* hmv_t;

typedef struct HashMapSt {
    hmv_t vec_vec;
    hmsize_t n;
    hmsize_t next_round;
} hm_s;



hm_s *new_hashmap();
void init_hmv(hm_s *self, hmsize_t index, hmsize_t len_seq);
void resize_insert_hmn(hmn_t self, hmsize_t index, seq_t seq, char *seq_str, hmsize_t len, size_t out_len);
void resize_insert_hmv(hmvalue_s *self, seq_t seq, char *seq_str, size_t out_len);
void resize_hashmap(hm_s *self);
void free_hashmap_vec(hmvalue_s self);
void free_hashmap(hm_s *self);
void insert_into_hashmap(hm_s *self, uint64_t key, seq_t  value, char *seq_str, hmsize_t len_seq, size_t out_len);
hmvalue_s get_hashmap_value(hm_s *self, uint64_t key, hmsize_t len_seq);

void print_hmv(hmvalue_s hmv);
void print_hmn(hm_node hmn);

