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
    3, 4, 4, 4, 1};

void swap(chartype_t *in, int i, int j)
{
    chartype_t tmp = in[i];
    in[i] = in[j];
    in[j] = tmp;
}

int quicksort_partition(chartype_t *in, int curr_l, int curr_h)
{
    chartype_t pivot = in[curr_h];

    int i = curr_l - 1;
    for (int j = curr_l; j < curr_h; j++)
    {
        if (in[j] < pivot)
        {
            i++;
            swap(in, i, j);
        }
    }

    swap(in, i + 1, curr_h);
    return i + 1;
}

void quicksort(chartype_t *in, int curr_l, int curr_h)
{
    if (curr_l < curr_h)
    {
        int partition_index = quicksort_partition(in, curr_l, curr_h);
        quicksort(in, curr_l, partition_index - 1);
        quicksort(in, partition_index + 1, curr_h);
    }
}

chartype_t pack_4_bytes_into_1_byte(chartype_t b1, chartype_t b2, chartype_t b3, chartype_t b4)
{
    chartype_t b11 = (3 & (b1 >> 1)) << 0;
    chartype_t b21 = (3 & (b2 >> 1)) << 2;
    chartype_t b31 = (3 & (b3 >> 1)) << 4;
    chartype_t b41 = (3 & (b4 >> 1)) << 6;

    chartype_t res = b11 | b21 | b31 | b41;
    return res;
}

void and_immintrin(chartype_t inn[], int overall_start_point, int start_point, int size, int k, chartype_t out[])
{
    __m256i in = _mm256_loadu_si256((__m256i *)&inn[0]);

    chartype_t to_be_anded[32];
    memset(to_be_anded, 0, 32);
    for (int i = start_point; i < start_point + k; i++)
        to_be_anded[i] = 0xff;
    __m256i and_candidate = _mm256_loadu_si256((__m256i *)&to_be_anded[0]);
    __m256i anded_arr = _mm256_and_si256(in, and_candidate);

    chartype_t v[32];
    chartype_t v2[32];
    _mm256_storeu_si256((__m256i *)&v[0], anded_arr);
    _mm256_storeu_si256((__m256i *)&v2[0], in);

    for (int i = start_point; i < start_point + k; i += 4)
    {
        chartype_t res = pack_4_bytes_into_1_byte(v[i + 0], v[i + 1], v[i + 2], v[i + 3]);
        out[(overall_start_point + start_point) / k] ^= res;
    }
}

void and_buffer(chartype_t buffer[SIZE_CHARS], chartype_t out[], int starting_point, int size, int k)
{

    for (int i = 0; i < SIZE_CHARS; i += k)
    {
        and_immintrin(buffer, starting_point, i, size, k, out);
    }
}

void get_kmers(chartype_t *in, chartype_t out[], int size, int k)
{
    if (k < 4 || k > 16 || k % 4 != 0)
    {
        printf("K must be between 4 and 16 and divisble by 4\n");
        exit(1);
    }

    char buffer[32];

    for (int i = 0; i < size; i += 32)
    {
        memset(buffer, 0, 32);
        memcpy(buffer, &in[i], 32);

        and_buffer(buffer, out, i, size, k);
    }
}

uint64_t pack_kmers_into_8_bytes(chartype_t kmers[], int size)
{
    uint64_t packed = 0;

    quicksort(kmers, 0, size - 1);
    for (int i = 0; i < 16; i += 2)
    {
        packed |= kmers[i] << i;
    }

    return packed;
}

uint64_t get_kmer_key(chartype_t *seq, int len_seq, int k)
{
    int size_out = (len_seq / k) + ((len_seq / k) % 16);
    chartype_t out[size_out];
    memset(out, 0, size_out);

    get_kmers(seq, out, len_seq, k);

    return pack_kmers_into_8_bytes(out, size_out);
}

void encode_gatacca(chartype_t in[SIZE_CHARS], outtype_t out[SIZE_OUT])
{
    __m256i all_threes = _mm256_set1_epi8(3);
    __m256i sl_nums = _mm256_setr_epi64x(0, 2, 4, 6);
    __m256i loaded_array = _mm256_loadu_si256((__m256i *)&in[0]);

    __m256i loaded_array_shifted_right = _mm256_srai_epi16(loaded_array, 1);
    __m256i loaded_array_anded = _mm256_and_si256(loaded_array_shifted_right, all_threes);
    __m256i loaded_array_shifted_left = _mm256_sllv_epi64(loaded_array_anded, sl_nums);

    outtype_t v[SIZE_OUT];

    _mm256_storeu_si256((__m256i *)&v[0], loaded_array_shifted_left);

    for (int i = 0; i < SIZE_OUT; i++)
    {
        out[i] = v[i];
    }
}

void reduce_integer_or_op(outtype_t in, outtype_t *reducer)
{
    *reducer = in | *reducer;
}

outtype_t pack_32_bytes_in_64_bits(chartype_t in[SIZE_CHARS])
{
    outtype_t out[SIZE_OUT];
    memset(out, 0, SIZE_OUT * sizeof(chartype_t));

    encode_gatacca(in, out);

    return out[0] | out[1] | out[2] | out[3];
    ;
}

/* seq must be null-terminated */
out_s pack_seq_into_64bit_integers(chartype_t *seq, size_t len_str)
{
    size_t size_padded = len_str < 32 ? 32 : next_round_bits32(len_str);
    size_t size_out = (size_padded / SIZE_CHARS) * sizeof(uint64_t);

    char seq_padded[size_padded];
    memset(seq_padded, 'A', size_padded);
    for (size_t i = 0; i < len_str; ++i)
        seq_padded[i] = seq[i];

    seq_t out = malloc(size_out);
    memset(out, 0, size_out);

    size_t j = 0;
    chartype_t in[SIZE_CHARS];
    for (size_t i = 0; i < size_padded; i += SIZE_CHARS)
    {
        memset(in, 0, SIZE_CHARS * sizeof(chartype_t));
        memcpy(in, &seq_padded[i], SIZE_CHARS * sizeof(chartype_t));

        out[j++] = pack_32_bytes_in_64_bits(in);
    }

    return (out_s){.out = out, .out_len = j};
}

/* seq must be null-terminated */
void insert_seq_in_hm(hm_s *self, chartype_t *seq, size_t index_in_array, int k)
{
    size_t len_str = strlen(seq);
    out_s packed_out = pack_seq_into_64bit_integers(seq, len_str);
    uint64_t key = get_kmer_key(seq, len_str, k);

    insert_into_hashmap(self, key, packed_out.out, len_str, packed_out.out_len, index_in_array);
}

hm_s *cluster_seqs(chartype_t **seqs_in, size_t num_seqs, int k)
{
    hm_s *hm = new_hashmap();

    for (int i = 0; i < num_seqs; i++)
    {
        insert_seq_in_hm(hm, seqs_in[i], i, k);
    }

    return hm;
}

/*
given a pointer to arrays of unsigned uint8_ts by Python, gets lazy hamming label. 1 for dup 0 for non-dup.
*/
int get_hamming_integers(hamtype_t a[SIZE_HAM], hamtype_t b[SIZE_HAM])
{
    __m256i a_loadedu = _mm256_loadu_si256((__m256i *)&a[0]);
    __m256i b_loadedu = _mm256_loadu_si256((__m256i *)&b[0]);

    __m256i xord = _mm256_xor_si256(a_loadedu, b_loadedu);

    uint8_t v[32];
    int diff = 0;
    uint8_t c;

    _mm256_storeu_si256((__m256i *)&v[0], xord);

    for (size_t i = 0; i < 32; i++)
    {
        c = v[i];
        diff += lookup_num_diffs[c];

        if (diff >= 2)
            break;
    }

    return diff;
}

int hamming_hseq_pair(clusterseq_s a, clusterseq_s b)
{
    int diff = 0;

    hamtype_t a_buffer[SIZE_HAM];
    hamtype_t b_buffer[SIZE_HAM];

    size_t num_bytes = SIZE_HAM * sizeof(hamtype_t);

    for (size_t i = 0; i < a.out_len; i += SIZE_HAM)
    {
        memcpy(a_buffer, &a.seq_packed[i], num_bytes);
        memcpy(b_buffer, &b.seq_packed[i], num_bytes);

        diff += get_hamming_integers(a_buffer, b_buffer);
    }

    return diff;
}

void *hamming_cluster_single(void *cluster_ptr)
{
    cluster_s *cluster = (cluster_s *)cluster_ptr;

    clusterseqarr_t cluster_seqs = cluster->arr;
    hmsize_t cluster_size = cluster->n;
    int diff = 0;

    clusterseq_s *lead;
    clusterseq_s *candidate;
    for (size_t i = 0; i < cluster_size; ++i)
    {
    set_diff:
        lead = &cluster_seqs[i];
        if (lead->is_dup)
            continue;
        diff = 0;

        for (size_t j = i + 1; j < cluster_size; ++j)
        {
            candidate = &cluster_seqs[j];
            if (candidate->is_dup)
                continue;

            diff = hamming_hseq_pair(*lead, *candidate);

            if (diff < 2)
            {
                insert_resize_dupe(lead, candidate);
                goto set_diff;
            }
        }
    }
}

/* seq must be null-terminated */
void hamming_clusters_hm(clusterarr_t non_zero_clusters, tuphash_t size)
{
    pthread_t threads[size];
    memset(threads, 0, size * sizeof(pthread_t));

    hmsize_t max = size;

    printf("Creating threads for each cluster...\n");
    for (hmsize_t i = 0; i < size; ++i)
    {
        pthread_create(&threads[i], NULL, &hamming_cluster_single, &non_zero_clusters[i]);
    }

    for (hmsize_t i = 0; i < size; i++) {
        pthread_join(threads[i], NULL);
        printf("Thread: `%u` joined; ", i);
    }

    printf("\n");
}

void iterate_and_mark_dups(clusterseq_s lead, int out[])
{
    if (lead.is_dup)
        return;

    size_t lead_index = lead.index_in_array;
    clusterseq_s *curr_dup;

    for (int i = 0; i < lead.size_dup; ++i)
    {
        curr_dup = lead.dupes[i];
        out[curr_dup->index_in_array] = lead_index;
    }
}

void mark_out(clusterarr_t clusters_arr, tuphash_t size, int out[])
{
    clusterseqarr_t curr_cluster_arr;
    clusterseq_s curr_seq;

    for (hmsize_t i = 0; i < size; ++i)
    {
        if (clusters_arr[i].n < 2)
            continue;
        curr_cluster_arr = clusters_arr[i].arr;

        for (hmsize_t j = 0; j < clusters_arr[i].n; ++j)
        {
            curr_seq = curr_cluster_arr[j];
            iterate_and_mark_dups(curr_seq, out);
        }
    }
}

non_zero_clusters_s filter_out_zero_clusters(clusterarr_t clusters, tuphash_t size)
{
    clusterarr_t ret = calloc(1, 1);
    size_t non_zero = 0;

    for (tuphash_t i = 0; i < size; i++)
    {
        if (clusters[i].n < 2)
            continue;

        non_zero++;
        ret = realloc(ret, non_zero * sizeof(cluster_s));
        ret[non_zero - 1] = clusters[i];
    }
    printf("Got %lu non-zero threads\n", non_zero);

    return (non_zero_clusters_s){.clusters = ret, .size = non_zero};
}

void cluster_ham_and_mark(chartype_t **seqs, size_t num_seqs, int k, int out[])
{
    printf("Bytes in i8/u8: %lu/%lu\n", sizeof(int8_t), sizeof(uint8_t));
    printf("Bytes in i16/u16: %lu/%lu\n", sizeof(int16_t), sizeof(uint16_t));
    printf("Bytes in i32/u32: %lu/%lu\n", sizeof(int32_t), sizeof(uint32_t));
    printf("Bytes in i64/u64: %lu/%lu\n", sizeof(int64_t), sizeof(uint64_t));
    printf("----\n");
    printf("Clustering...\n");
    hm_s *clustered = cluster_seqs(seqs, num_seqs, k);
    printf("Done, getting non-szero clusters...\n");
    non_zero_clusters_s non_zeroes = filter_out_zero_clusters(clustered->vec_vec, clustered->n);
    printf("Doing hamming...\n");
    hamming_clusters_hm(non_zeroes.clusters, non_zeroes.size);
    printf("Markint the results...\n");
    mark_out(non_zeroes.clusters, non_zeroes.size, out);

    printf("Fully done!\n");
    free_hashmap(clustered);
}

void insert_resize_dupe(clusterseq_s *self, clusterseq_s *dupe)
{
    self->size_dup++;

    clusterseq_s **resized = realloc(self->dupes, sizeof(clusterseq_s) * self->size_dup);

    if (!resized)
    {
        printf("Error reallocating dupe array\n");
        exit(139);
    }

    self->dupes = resized;
    self->dupes[self->size_dup - 1] = dupe;
    dupe->is_dup = 1;
}

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
    return ROUNDUP_16(n);
}

tuphash_t hash_tuple_to_index(uint64_t x, hmsize_t len)
{
    hmsize_t hash_x = hash_bits(x);

    hmsize_t hashed = (tuphash_t)((((hash_x ^ PYHASH_X) % PYHASH_REM1) ^ (PYHASH_X ^ (len >> 2))) % (PYHASH_REM2));
    return (hashed % HASH_MAX) + 1;
}

hm_s *new_hashmap()
{
    hm_s *hm = malloc(sizeof(hm_s));

    if (!hm)
    {
        printf("Error allocating hashmap on heap\n");
        exit(139);
    }

    hm->vec_vec = (cluster_s *)calloc(1, sizeof(cluster_s));
    hm->n = 0;
    hm->next_round = 0;

    return hm;
}
void init_hmv(hm_s *self, tuphash_t index, hmsize_t len_seq)
{
    self->vec_vec[index] = (cluster_s){.arr = malloc(sizeof(clusterseq_s)), .len_seq = len_seq, .n = 0x00000000, .hash = index + 1};
}

void resize_insert_hmn(clusterseqarr_t self, hmsize_t index, seq_t seq_packed, size_t out_len, size_t index_in_array)
{
    self[index - 1] = (clusterseq_s){.dupes = calloc(1, sizeof(clusterseq_s)), .seq_packed = seq_packed, .out_len = out_len, .size_dup = 0, .index_in_array = index_in_array, .is_dup = 0};
}

void resize_insert_hmv(cluster_s *self, seq_t seq_packed, size_t out_len, size_t index_in_array)
{
    self->n++;
    size_t new_len = sizeof(clusterseq_s) * self->n;
    self->arr = (clusterseqarr_t)realloc(self->arr, new_len);
    resize_insert_hmn(self->arr, self->n, seq_packed, out_len, index_in_array);
}

void resize_hashmap(hm_s *self)
{
    hmsize_t rounded_up = self->next_round = next_round_bits32(self->n);
    if (rounded_up == 0)
        rounded_up = self->next_round = UINT16_MAX;

    if (rounded_up > self->n)
    {
        cluster_s *p_new = (cluster_s *)realloc(self->vec_vec, rounded_up * sizeof(cluster_s));
        if (!p_new)
        {
            printf("Error reallocating hashmap value vector on heap to %hu\n", self->next_round);
            exit(139);
        }
        self->vec_vec = p_new;
    }
}

void free_hashmap_node(clusterseq_s node)
{
    free(node.seq_packed);
}

void free_hashmap_vec(cluster_s self)
{
    for (int i = 0; i < self.n; i++)
    {
        free_hashmap_node(self.arr[i]);
    }
}

void free_hashmap(hm_s *self)
{
    for (hmsize_t i = 0; i < self->n; ++i)
        free_hashmap_vec(self->vec_vec[i]);

    free(self);
}

void insert_into_hashmap(hm_s *self, uint64_t key, seq_t seq_packed, size_t len_seq, size_t out_len, size_t index_in_array)
{
    tuphash_t vec_index = hash_tuple_to_index(key, len_seq);
    if (self->n < vec_index)
    {
        self->n = vec_index;
        resize_hashmap(self);

        init_hmv(self, vec_index - 1, len_seq);
    }

    resize_insert_hmv(&self->vec_vec[vec_index - 1], seq_packed, out_len, index_in_array);
}

cluster_s get_hashmap_value(hm_s *self, uint64_t key, hmsize_t len_seq)
{
    size_t vec_index = hash_tuple_to_index(key, len_seq);

    return self->vec_vec[vec_index - 1];
}
