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


typedef struct HashMapNode {
    seq_t seq_packed;
    size_t out_len;
    size_t index_in_array;
    int is_dup;
    int set_fifty;
} clusterseq_s;
typedef clusterseq_s* clusterseqarr_t;

typedef struct HashMapValue {
    clusterseq_s *clusterseq_arr;
    hmsize_t len_seq;
    hmsize_t n;
    hmsize_t next_round;
    tuphash_t hash;
    int set_fifty;
} cluster_s;
typedef cluster_s* clusterarr_t;


typedef struct HashMapBucket {
    tuphash_t hash;
    cluster_s *cluster_arr;
    tuphash_t n;
    tuphash_t next_round;
    int set_fifty;
} bucket_s;


typedef struct HashMapSt {
    bucket_s *bucket_arr; 
    tuphash_t n;
    tuphash_t next_round;
} hm_s;


tuphash_t hash_tuple_to_index(uint64_t x, hmsize_t len);
tuphash_t next_round_bits16(tuphash_t n);
hmsize_t next_round_bits32(hmsize_t n);
hmsize_t hash_bits(uint64_t x);
hm_s new_hashmap();
bucket_s new_bucket(tuphash_t hash);
cluster_s new_cluster(tuphash_t hash, hmsize_t len);
clusterseq_s new_clusterseq(seq_t seq_packed, size_t out_len, size_t index_in_array);
void resize_insert_bucket(hm_s *self, tuphash_t hash);
void resize_insert_cluster(bucket_s *self, tuphash_t hash, hmsize_t len);
void resize_insert_clusterseq(cluster_s *self, seq_t seq_packed, size_t out_len, size_t index_in_array);
void insert_seq_into_hashmap(hm_s *self, uint64_t key, seq_t seq, hmsize_t len_seq, hmsize_t out_len, size_t index_in_array);
uint64_t djb2(seq_t seq, int size);
void free_clusterseq(clusterseq_s *self);
void free_cluster(cluster_s *self);
void free_bucket(bucket_s *self);
void free_hashmap(hm_s *self);