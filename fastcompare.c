#include "fastcompare.h"

const uint8_t lookup_num_diffs[257] = {
    0, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 
    1, 2, 2, 2, 1, 2, 2, 2, 2, 3, 3, 3, 
    2, 3, 3, 3, 2, 3, 3, 3, 1, 2, 2, 2, 
    2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 
    1, 2, 2, 2, 2, 3, 3, 3, 2, 3, 3, 3, 
    2, 3, 3, 3, 1, 2, 2, 2, 2, 3, 3, 3, 
    2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 
    3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 
    2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 
    3, 4, 4, 4, 2, 3, 3, 3, 3, 4, 4, 4, 
    3, 4, 4, 4, 3, 4, 4, 4, 1, 2, 2, 2, 
    2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 
    2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 
    3, 4, 4, 4, 2, 3, 3, 3, 3, 4, 4, 4, 
    3, 4, 4, 4, 3, 4, 4, 4, 2, 3, 3, 3, 
    3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 
    1, 2, 2, 2, 2, 3, 3, 3, 2, 3, 3, 3, 
    2, 3, 3, 3, 2, 3, 3, 3, 3, 4, 4, 4, 
    3, 4, 4, 4, 3, 4, 4, 4, 2, 3, 3, 3, 
    3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 
    2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4,
    3, 4, 4, 4, 1
};


void encode_gatacca(chartype_t in[SIZE_CHARS], outtype_t out[SIZE_OUT]) {
    __m256i all_threes = _mm256_set1_epi8(3);
    __m256i sl_nums = _mm256_setr_epi64x(0, 2, 4, 6);
    __m256i loaded_array = _mm256_loadu_si256((__m256i *)&in[0]);

    __m256i loaded_array_shifted_right =  _mm256_srai_epi16(loaded_array, 1);
;   __m256i loaded_array_anded = _mm256_and_si256(loaded_array_shifted_right, all_threes);
    __m256i loaded_array_shifted_left = _mm256_sllv_epi64(loaded_array_anded, sl_nums);

  
    outtype_t v[SIZE_OUT];

    _mm256_storeu_si256((__m256i*)&v[0], loaded_array_shifted_left);

    for (int i = 0; i < SIZE_OUT; i++) {
        out[i] = v[i];
    }

}

void reduce_integer_or_op(outtype_t in, outtype_t *reducer) {
    *reducer = in | *reducer;
}

outtype_t pack_32_bytes_in_64_bits(chartype_t in[SIZE_CHARS]) {
    outtype_t out[SIZE_OUT];
    memset(out, 0, SIZE_OUT * sizeof(chartype_t));

    encode_gatacca(in, out);

    return out[0] | out[1] | out[2] | out[3];;
}

/* seq must be null-terminated */
out_s pack_seq_into_64bit_integers(chartype_t *seq) {
    size_t len_str = strlen(seq);
    size_t size_padded = len_str + (len_str % SIZE_CHARS);
    size_t size_out = (size_padded / SIZE_CHARS) * sizeof(uint64_t);

    seq_t out = malloc(size_out);
    memset(out, 0, size_out);

    size_t j = 0;
    chartype_t in[SIZE_CHARS];
    for (size_t i = 0; i < size_padded; i += SIZE_CHARS) {
        memset(in, 0, SIZE_CHARS * sizeof(chartype_t));
        memcpy(in, &seq[i], SIZE_CHARS * sizeof(chartype_t));

        out[j++] = pack_32_bytes_in_64_bits(in);
    }

    return (out_s){.out=out; out_len=j};
}


/* seq must be null-terminated */
void insert_seq_in_hm(hm_s *self, char *seq) {
    hmsize_t len_seq = (hmsize_t)strlen(seq);
    out_s packed_out = pack_seq_into_64bit_integers(seq);

    insert_into_hashmap(self, packed_out.out[0], packed_out.out, seq, len_seq, packed_out.len_out);
} 

hm_s *cluster_seqs(char **seqs_in, size_t num_seqs) {
    hm_s hm = new_hashmap();

    for (int i = 0; i < num_seqs; i++) {
        insert_seq_in_hm(hm, seqs_in[i]);
    }

    return hm;
}



/*
given a pointer to arrays of unsigned uint8_ts by Python, gets lazy hamming label. 1 for dup 0 for non-dup.
*/
int get_hamming_integers(hamtype_t a[SIZE_HAM], hamtype_t b[SIZE_HAM]) {  
    __m256i a_loadedu = _mm256_loadu_si256((__m256i*)&a[0]);
    __m256i b_loadedu = _mm256_loadu_si256((__m256i*)&b[0]);

    __m256i xord = _mm256_xor_si256(a_loadedu, b_loadedu);

    uint8_t v[32];
    int diff = 0;
    uint8_t c;

    _mm256_storeu_epi8((__m256i*)&v[0], xord);

    for (size_t i = 0; i < 32; i++) {
        c = v[i];
        diff += lookup_num_diffs[c];

        if (diff >= 2) break;
    }

    return diff;

}


void get_hamming_cluster(hamtype_t *in_cluster, size_t len_rows, int *out) {
    hamtype_t (*in_2d)[len_rows][SIZE_HAM] = (hamtype_t (*)[len_rows][SIZE_HAM])in_cluster;

    hamtype_t a[SIZE_HAM];
    hamtype_t b[SIZE_HAM];
    int64_t i = -1;
    int64_t j = 0;
    int out_i = 0;
    int out_j = 0;
    int diff = 0;

    while (i++ < (int64_t)len_rows) {
        set_j:
        diff = 0;
        out_i = out[i];
        if (out_i != -1) continue;
        j = i + 1;

        while (j < (int64_t)len_rows) {
            out_j = out[j];
            if (out_j != -1) { j++; continue; }
  
            memcpy(a, in_2d[i], SIZE_HAM * sizeof(hamtype_t));
            memcpy(b, in_2d[j], SIZE_HAM * sizeof(hamtype_t));

            diff = get_hamming_integers(a, b); 
            if (diff < 2) {
                out[j] = i;
                goto set_j;                
            }
            j++;
        }
    }

}


hmsize_t hash_bits(uint64_t x) {
  hmsize_t low = (hmsize_t)x;
  hmsize_t high = (hmsize_t)(x >> HM_SHIFT);
  return (hmsize_t)((A * low + B * high + C) >> HM_SHIFT);
}

hmsize_t next_round_bits(hmsize_t n) {
    return ROUNDUP_32(n);
}


hmsize_t hash_tuple_to_index(uint64_t x, hmsize_t len) {
    hmsize_t hash_x = hash_bits(x);

    hmsize_t ret = (hmsize_t)((((hash_x ^ PYHASH_X) % PYHASH_REM1) ^ (PYHASH_X ^ (len >> 2))) % (PYHASH_REM2));
    return ret; 
}

hm_s *new_hashmap() {
    hm_s *hm = malloc(sizeof(hm_s));

    if (!hm) {
        printf("Error allocating hashmap on heap\n");
        exit(139);
    }

    hm->vec_vec = (hmvalue_s*)calloc(1, sizeof(hmvalue_s));
    hm->n = 0;
    hm->next_round = 0;

    return hm;

}
void init_hmv(hm_s *self, hmsize_t index, hmsize_t len_seq) {
    self->vec_vec[index] = (hmvalue_s){.arr=malloc(sizeof(hm_node)), .len_seq=len_seq, .n=0};
}

void resize_insert_hmn(hmn_t self, hmsize_t index, seq_t seq, char *seq_str, hmsize_t len, , size_t out_len) {
    self[index - 1] = (hm_node){.seq = seq, .len_seq=len, .seq_str=seq_str, .len_out=out_len};   
}

void resize_insert_hmv(hmvalue_s *self, seq_t seq, char *seq_str, size_t out_len) {
    self->n++;
    self->arr = (hmn_t)realloc(self->arr, sizeof(hm_node) * self->n);
    resize_insert_hmn(self->arr, self->n, seq, seq_str, self->len_seq, out_len);
}

void resize_hashmap(hm_s *self) {
    hmsize_t rounded_up = self->next_round = next_round_bits(self->n);

    if (rounded_up > self->n) {
        hmvalue_s *p_new = (hmvalue_s*)realloc(self->vec_vec, rounded_up * sizeof(hmvalue_s));
        if (!p_new) {
            printf("Error reallocating hashmap value vector on heap to %hu\n", self->next_round);
            exit(139);
        }
        self->vec_vec = p_new;
    }
}


void free_hashmap_vec(hmvalue_s self) {
    for (int i = 0; i < self.n; i++) {
        free(self.arr[i].seq);
    }
}

void free_hashmap(hm_s *self) {
    for (hmsize_t i = 0; i < self->n; ++i) free_hashmap_vec(self->vec_vec[i]);

    free(self);
}

void insert_into_hashmap(hm_s *self, uint64_t key, seq_t  value, char *seq_str, hmsize_t len_seq, size_t out_len) {
    hmsize_t vec_index = hash_tuple_to_index(key, len_seq);

    if (self->n < vec_index) {
        self->n = vec_index;
        resize_hashmap(self);

        init_hmv(self, vec_index - 1, len_seq);
    }

    resize_insert_hmv(&self->vec_vec[vec_index - 1], value, out_len);
}

hmvalue_s get_hashmap_value(hm_s *self, uint64_t key, hmsize_t len_seq) {
    size_t vec_index = hash_tuple_to_index(key, len_seq);

    return self->vec_vec[vec_index - 1];
}

void print_hmv(hmvalue_s hmv) {
    for (int i = 0; i < hmv.n; i++) print_hmn(hmv.arr[i]);
}
void print_hmn(hm_node hmn) {
    for (int i = 0; i < hmn.len_seq; i++) printf("%lu/%lx, ", hmn.seq[i], hmn.seq[i]);

    printf("\n------\n");
}

int main() {
    hm_s *hm = new_hashmap();

    uint64_t key = 0x010101ddeeff1142;
    uint64_t seq[] = {0x010101ddeeff1142, 0xf10101ddeeff1e42, 0x010101ddeefe1143,  0x01e101fdefff1142};

    insert_into_hashmap(hm, key, seq, 4);

    hmvalue_s v = get_hashmap_value(hm, key, 4);

    print_hmv(v);

}