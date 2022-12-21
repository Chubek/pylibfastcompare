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

tuphash_t hash_tuple_to_index(uint64_t x, hmsize_t len)
{
    hmsize_t hash_x = hash_bits(x);
    hmsize_t hashed = (tuphash_t)((((hash_x ^ PYHASH_X) % PYHASH_REM1) ^ (PYHASH_X ^ (len >> 2))) % (PYHASH_REM2));
    
    hashed = ((hashed % HASH_MAX) + 1);

    tuphash_t h1 = (tuphash_t)hashed;
    tuphash_t h2 = (tuphash_t)(hashed >> 16);
    
    return h1 ^ h2;
}


hm_s new_hashmap() {
    return (hm_s){.bucket_arr=calloc(HASH_MAX, sizeof(bucket_s)), .n=0, .next_round=HASH_MAX};
}

bucket_s new_bucket(tuphash_t hash) {
    return (bucket_s){.cluster_arr=calloc(HASH_MAX, sizeof(cluster_s)), .hash=hash, .n=0, .next_round=HASH_MAX, .set_fifty=50};
}

cluster_s new_cluster(tuphash_t hash, hmsize_t len) {
    return (cluster_s){.clusterseq_arr=calloc(8, sizeof(clusterseq_s)), .hash=hash, .len_seq=len, .n=0, .next_round=8, .set_fifty=50};
}

clusterseq_s new_clusterseq(seq_t seq_packed, size_t out_len, size_t index_in_array) {
    return (clusterseq_s){.seq_packed=seq_packed, .out_len=out_len, .index_in_array=index_in_array, .is_dup=0, .set_fifty=50};
}


void free_clusterseq(clusterseq_s *self) {
    free(self->seq_packed);
}

void free_cluster(cluster_s *self) {
    for (int i = 0; i < self->next_round; i++) {
        if (self->set_fifty != 50) continue;
        free_clusterseq(&self->clusterseq_arr[i]);
    }

}

void free_bucket(bucket_s *self) {
    for (int i = 0; i < self->next_round; i++) {
        if (self->set_fifty != 50) continue;
        free_cluster(&self->cluster_arr[i]);
    }

}

void free_hashmap(hm_s *self) {
    for (int i = 0; i < self->next_round; i++) {
        free_bucket(&self->bucket_arr[i]);
    }
}

void resize_insert_bucket(hm_s *self, tuphash_t hash) {
    tuphash_t next_round = next_round_bits16(hash);

    if (next_round > self->next_round) {
        bucket_s *nptr = (bucket_s *)realloc(self->bucket_arr, next_round * sizeof(bucket_s));

        if (!nptr) {
            printf("Error reallcating bucket array.\n");
            exit(ENOMEM);
        }

        self->bucket_arr = nptr;
        self->next_round = next_round;
    }

    self->bucket_arr[hash - 1] = new_bucket(hash);
    self->n++;
}

void resize_insert_cluster(bucket_s *self, tuphash_t hash, hmsize_t len) {
    tuphash_t next_round = next_round_bits16(hash);

    if (next_round > self->next_round) {
        cluster_s  *nptr = (cluster_s *)realloc(self->cluster_arr, next_round * sizeof(bucket_s));

        if (!nptr) {
            printf("Error reallcating cluster array.\n");
            exit(ENOMEM);
        }

        self->cluster_arr = nptr;
        self->next_round = next_round;
    }

    self->cluster_arr[hash - 1] = new_cluster(hash, len);
    self->n++;
}

void resize_insert_clusterseq(cluster_s *self, seq_t seq_packed, size_t out_len, size_t index_in_array) {
    tuphash_t next_round = next_round_bits16(self->n);

    if (next_round > self->next_round) {
        clusterseq_s *nptr = (clusterseq_s *)realloc(self->clusterseq_arr, next_round * sizeof(bucket_s));

        if (!nptr) {
            printf("Error reallcating clusterseq array.\n");
            exit(ENOMEM);
        }

        self->clusterseq_arr = nptr;
        self->next_round = next_round;
    }

    self->clusterseq_arr[self->n] = new_clusterseq(seq_packed, out_len, index_in_array);
    self->n++;
}

void insert_seq_into_hashmap(hm_s *self, uint64_t key, seq_t seq, hmsize_t len_seq, hmsize_t out_len, size_t index_in_array) {
    tuphash_t hash_bucket = hash_tuple_to_index(key, len_seq);
    tuphash_t hash_cluster = hash_tuple_to_index(key, out_len);

    if (hash_bucket > self->next_round) {
        resize_insert_bucket(self, hash_bucket);
    }

    bucket_s *bucket = &self->bucket_arr[hash_bucket - 1];
    bucket->hash = hash_bucket;
    bucket->set_fifty = 50;

    if (hash_cluster > bucket->next_round) {
        resize_insert_cluster(bucket, hash_cluster, len_seq);
    }

    cluster_s *cluster = &bucket->cluster_arr[hash_cluster - 1];
    cluster->hash = hash_cluster;
    cluster->set_fifty = 50;


    resize_insert_clusterseq(cluster, seq, out_len, index_in_array);
}