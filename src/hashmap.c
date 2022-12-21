#include "../include/fastcompare.h"

hmsize_t hash_bits(uint64_t x)
{
    hmsize_t low = (hmsize_t)x;
    hmsize_t high = (hmsize_t)(x >> HM_SHIFT);
    return (hmsize_t)((A * low + B * high + C) >> HM_SHIFT) + 1;
}

hmsize_t next_round_bits32(hmsize_t n)
{
    return ROUNDUP_32(n);
}

tuphash_t next_round_bits16(tuphash_t n)
{
    int rounded_up = ROUNDUP_16(n);
    
    if (rounded_up == 0) rounded_up = UINT16_MAX;

    return rounded_up;
}

tuphash_t hash_tuple_to_hm_index(uint64_t x, hmsize_t len)
{
    hmsize_t hash_x = hash_bits(x);
    hmsize_t hashed = (tuphash_t)((((hash_x ^ PYHASH_X) % PYHASH_REM1) ^ (PYHASH_X ^ (len >> 2))) % (PYHASH_REM2));
    
    hashed = ((hashed % HASH_MAX) + 1);

    tuphash_t h1 = (tuphash_t)hashed;
    tuphash_t h2 = (tuphash_t)(hashed >> 16);
    
    return h1 ^ h2;
}

buckethash_t hash_tuple_to_bucket_index(uint64_t x, hmsize_t len) {
    tuphash_t hash_x = hash_bits(x);
    tuphash_t hashed = (tuphash_t)((((hash_x ^ PYHASH_X) % PYHASH_REM1) ^ (PYHASH_X ^ (len >> 2))) % (PYHASH_REM2));
    
    hashed = ((hashed % BUCKET_HASH_MAX) + 1);

    buckethash_t lower_four_bits = hashed << 14;
    buckethash_t upper_four_bits = hashed >> 14;

    return ((buckethash_t)hashed & 0xFF) ^ (~lower_four_bits & upper_four_bits);
}


hm_s new_hashmap() {
    hm_s hm = (hm_s){.bucket_arr=calloc(HASH_MAX, SZ_BUCKET), .n=0};
    memset(hm.occupied, 0, HASH_MAX);

    return hm;
}

void new_bucket(bucket_s *self, tuphash_t hash) {
    self->cluster_arr = calloc(BUCKET_HASH_MAX, SZ_CLST);
    self->hash = hash;
    self->n = 0;

    memset(self->occuped, 0, BUCKET_HASH_MAX);
}

void new_cluster(cluster_s *self, buckethash_t hash, hmsize_t len) {
    self->clusterseq_arr = NULL;
    self->hash = hash;
    self->len_seq = len;
    self->n = 0;
}

void new_clusterseq(clusterseq_s *self, seq_t seq_packed, size_t out_len, size_t index_in_array) {
    self->seq_packed = seq_packed;
    self->out_len = out_len;
    self->index_in_array = index_in_array;
    self->is_dup = 0;
}


void init_bucket(hm_s *self, tuphash_t hash) {
    new_bucket(&self->bucket_arr[hash - 1], hash);
    self->occupied[hash - 1] = OCCUPIED;
    self->n++;
}

void init_cluster(bucket_s *self, buckethash_t hash, hmsize_t len) {
    new_cluster(&self->cluster_arr[hash - 1], hash, len);
    self->occuped[hash - 1] = OCCUPIED;
    self->n++;
}

void resize_insert_clusterseq(cluster_s *self, seq_t seq_packed, size_t out_len, size_t index_in_array) {
    clusterseq_s *nptr = (clusterseq_s *)realloc(self->clusterseq_arr, ++self->n * SZ_CLSQ);
    
    if (!nptr) {
        printf("Error reallcating clusterseq array.\n");
        exit(ENOMEM);
    }

    self->clusterseq_arr = nptr;    
    new_clusterseq(&self->clusterseq_arr[self->n - 1], seq_packed, out_len, index_in_array);
}

void insert_seq_into_hashmap(hm_s *self, uint64_t key, seq_t seq, hmsize_t len_seq, hmsize_t out_len, size_t index_in_array) {
    tuphash_t hash_bucket = hash_tuple_to_hm_index(key, len_seq);
    buckethash_t hash_cluster = hash_tuple_to_bucket_index(key, out_len);
    
    bucket_s *bucket = &self->bucket_arr[hash_bucket - 1];
    if (self->occupied[hash_bucket - 1] != OCCUPIED) {
        init_bucket(self, hash_bucket);
    }

    cluster_s *cluster = &bucket->cluster_arr[hash_cluster - 1];
    if (bucket->occuped[hash_cluster - 1] != OCCUPIED) {
        init_cluster(bucket, hash_cluster, len_seq);
    }

    resize_insert_clusterseq(cluster, seq, out_len, index_in_array);
}

