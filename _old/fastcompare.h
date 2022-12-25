#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#define K 4
#define NUM_PARA 48
#define QBUFF   2048

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
#define fifo_MAX 6000
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

#define HASH_MAX UINT16_MAX
#define SZ_MAX 256
#define THREAD_CHUNK 2000


#define ROUNDUP_32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#define ROUNDUP_16(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, ++(x))

typedef uint8_t chartype_t;
typedef uint64_t outtype_t;
typedef uint8_t hamtype_t;
typedef uint16_t tuphash_t;
typedef uint8_t* seq_t;
typedef uint32_t hmsize_t;


typedef struct OutStruct {
    seq_t out;
    size_t out_len;
} out_s;


typedef struct HashMapNode {
    seq_t seq_packed;
    size_t out_len;
    size_t index_in_array;
    int is_dup;
} clusterseq_s;
typedef clusterseq_s* clusterseqarr_t;

typedef struct HashMapValue {
    clusterseq_s *clusterseq_arr;
    hmsize_t len_seq;
    hmsize_t n;
    hmsize_t next_round;
    tuphash_t hash;
} cluster_s;
typedef cluster_s* clusterarr_t;


typedef struct HashMapBucket {
    tuphash_t hash;
    cluster_s *cluster_arr;
    tuphash_t n;
    tuphash_t next_round;
} bucket_s;


typedef struct HashMapSt {
    bucket_s *bucket_arr; 
    tuphash_t n;
    tuphash_t next_round;
} hm_s;


typedef struct NonZeroClusters {
    clusterarr_t clusters;
    tuphash_t size;
} non_zero_clusters_s;


typedef struct PairWiseSeq {
    clusterseq_s *lead;
    clusterseq_s *candidate;
    int *skip_rest;
} pairwise_s;


typedef struct fifoQueue {
    pairwise_s arr[QBUFF];
    int curr_index;
    int has_member;
    int join;
    pthread_mutex_t lock;
} fifo_s;
