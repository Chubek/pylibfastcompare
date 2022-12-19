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

int *global_out;
fifo_s queues[NUM_PARA];

pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

int sleepms(long miliseconds)
{
   struct timespec rem;
   struct timespec req= {
       (int)(miliseconds / 1000),     /* secs (Must be Non-Negative) */ 
       (miliseconds % 1000) * 1000000 /* nano (Must be in range of 0 to 999999999) */ 
   };

   return nanosleep(&req , &rem);
}

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


chartype_t pack_1_byte_into_8_bits(chartype_t enc1, chartype_t enc2, chartype_t enc3, chartype_t enc4) {
    chartype_t enc2_sld = enc2 << 2;
    chartype_t enc3_sld = enc3 << 4;
    chartype_t enc4_sld = enc4 << 6;

    return enc1 | enc2_sld | enc3_sld | enc4_sld;
}


void encode_to_0123(chartype_t in[32], chartype_t out[32]) {
    __m256i three = _mm256_set1_epi8(3);

    __m256i loaded_sri = _mm256_loadu_si256((__m256i*)&in[0]);

    __m256i srd = _mm256_srai_epi16(loaded_sri, 1);
    __m256i andd = _mm256_and_si256(srd, three);

    _mm256_storeu_si256((__m256i*)&out[0], andd);   
}

void and_buffer(chartype_t buffer[SIZE_CHARS], chartype_t out[], int starting_point, int size, int k)
{
    chartype_t enc1;
    chartype_t enc2;
    chartype_t enc3;
    chartype_t enc4;
    
    chartype_t out_buffer[32];
    memset(out_buffer, 0, 32);
    
    encode_to_0123(buffer, out_buffer);

    for (int i = 0; i < SIZE_CHARS; i += k)
    {                
        enc1 = out_buffer[i];
        enc2 = out_buffer[i + 1];
        enc3 = out_buffer[i + 2];
        enc4 = out_buffer[i + 3];
        out[(starting_point + i) / k] = pack_1_byte_into_8_bits(enc1, enc2, enc3, enc4);
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
        for (int j = 0; j < 32; j++) buffer[j] = in[j + i];

        and_buffer(buffer, out, i, size, k);
    }
}

void get_freq_value(chartype_t *in, int size_in, int uint8_freqs[256]) {
    memset(uint8_freqs, 0, 256 * sizeof(int));
    
    for (int i = 0; i < size_in; i++) {
        uint8_freqs[in[i]] += 1;
    }
}


uint64_t murmurhash(const uint64_t key, int non_zero, uint32_t seed) {
    const uint64_t m = 0xc6a4a7935bd1e995;
	const int r = 47;

    uint64_t h = seed ^ (non_zero * m);
    uint64_t hprime = 0;

    const uint64_t *data = &key;
    const uint64_t *end = data + (non_zero / 8);

    while (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char *data2 = (const unsigned char*)data;

    switch (non_zero & 7) {
        case 7: h ^= ((uint64_t)data2[6]) << 48;
	    case 6: h ^= ((uint64_t)data2[5]) << 40;
	    case 5: h ^= ((uint64_t)data2[4]) << 32;
	    case 4: h ^= ((uint64_t)data2[3]) << 24;
	    case 3: h ^= ((uint64_t)data2[2]) << 16;
	    case 2: h ^= ((uint64_t)data2[1]) << 8;
	    case 1: h ^= ((uint64_t)data2[0]);
	        h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    if (key % 2 != 0) {
        hprime = h;
        h >>= 18; h *= m;
        h >>= 22; h += m;
        h >>= 17; h *= m;
        h ^= hprime; h >>= 2;
    }


    return h;
}

uint64_t merge_freqs(int freqs[256]) {
    uint64_t key = 0;


    int sum = 0;
    int non_zero_num = 1;
    for (int i = 0; i < 256; i++) {
        if (freqs[i] == 0) continue;        
        sum += freqs[i];
        non_zero_num += 1;
    }

    key = sum / non_zero_num;

    key <<= 16;

    return murmurhash(key, non_zero_num, (uint32_t)(key >> 32));
}

uint64_t get_kmer_key(seq_t out, int size_out, int k){
    int freqs[256];
    get_freq_value(out, size_out, freqs);
    return merge_freqs(freqs);
}

uint64_t djb2(seq_t seq, int size) {
    uint64_t hash = 5381UL;

    for (int i = size / 2; i < 3 * (size / 4); i++) {
        hash = ((hash << 5) + hash) + seq[i];
    }

    return hash;
}

/* seq must be null-terminated */
void insert_seq_in_hm(hm_s *self, chartype_t *seq, size_t index_in_array, int k)
{
    size_t len_seq = strlen(seq);
    int size_out = (len_seq / k);
    int size_padded = size_out + (size_out % 32);
    seq_t out = (seq_t)calloc(size_padded, sizeof(chartype_t));

    get_kmers(seq, out, len_seq, k);
    uint64_t key = get_kmer_key(out, size_out, k);
    uint64_t djb = djb2(seq, len_seq);

    key ^= djb >> 48;

    insert_into_hashmap(self, key, out, len_seq, size_out, index_in_array);
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
        int dd = lookup_num_diffs[c];
        diff += lookup_num_diffs[c];
        if (diff >= 2)
            break;
    }
    return diff;
}

int hamming_hseq_pair(clusterseq_s a, clusterseq_s b)
{
    int diff = 0;
    int new_diff = 0;

    hamtype_t a_buffer[SIZE_HAM];
    hamtype_t b_buffer[SIZE_HAM];
    size_t num_bytes = SIZE_HAM * sizeof(hamtype_t);    

    seq_t a_seq = a.seq_packed;
    seq_t b_seq = b.seq_packed;
    int out_len = strlen(a_seq);

    for (size_t i = 0; i < out_len; i += SIZE_HAM)
    {

        memset(a_buffer, 0, num_bytes);
        memset(b_buffer, 0, num_bytes);        
        
        for (int j = 0; j < (i > (out_len - (out_len % 32)) ? out_len % 32 : 32); j++) a_buffer[i] = a_seq[i + j];
        for (int j = 0; j < (i > (out_len - (out_len % 32)) ? out_len % 32 : 32); j++) b_buffer[i] = b_seq[i + j];

        new_diff = get_hamming_integers(a_buffer, b_buffer);
        diff += new_diff;
    }
 
    return diff;
}

void *parallel_pairwise_comparator(void *num) {
    int *np = (int*)num;
    int n = *np;
    int diff;
    pairwise_s curr_pair;  
    fifo_s *q = &queues[n];

    do {
        if (q->has_member) {
            sleepms(1L);
            curr_pair = pop_fifo(q);

            if (!curr_pair.candidate) continue;
            if (!curr_pair.lead) continue;
            if (curr_pair.candidate->is_dup) continue;

            int diff = hamming_hseq_pair(*curr_pair.lead, *curr_pair.candidate);
            if (diff < 2) {
                pthread_mutex_lock(&global_lock);
                global_out[curr_pair.candidate->index_in_array] = curr_pair.lead->index_in_array;
                pthread_mutex_unlock(&global_lock);
                curr_pair.candidate->is_dup = 1;
                *curr_pair.skip_rest = 1;

                sleepms(1L);
            }

        }
    } while(!q->join);

}


void *hamming_cluster_single(void *cluster_ptr)
{
    cluster_s *cluster = (cluster_s *)cluster_ptr;

    clusterseqarr_t cluster_seqs = cluster->arr;
    hmsize_t cluster_size = cluster->n;

    int diff = 0;

    clusterseq_s *lead;
    clusterseq_s *candidate;
    int i = 0;

    do
    {
        lead = &cluster_seqs[i];
        if (lead->is_dup)
            continue;     

        anew:   
        int k = 0;
        int skip_rest = 0;
        int j = i + 1;

        do 
        {
            if (k > NUM_PARA - 1) k = 0;
            if (skip_rest) {
                printf("Sequence #%d from cluster with size %d deduped\n", i, cluster->n);
                goto anew;
            };       
            candidate = &cluster_seqs[j];            
            
            lock_fifo(&queues[k]);
            put_fifo(&queues[k], (pairwise_s){.lead=lead, .candidate=candidate, .skip_rest=&skip_rest});
            unlock_fifo(&queues[k]);

            k++;

            sleepms(1L);
        } while (j++ < cluster_size);
    } while (i++ < cluster_size);
}

/* seq must be null-terminated */
void hamming_clusters_hm(clusterarr_t non_zero_clusters, tuphash_t size)
{
    if (pthread_mutex_init(&global_lock, NULL) != 0) {
        printf("Failed to initiailize global thread lock. Exiting...\n");
        exit(1);
    }

    pthread_t workers[NUM_PARA];
    int nums[NUM_PARA];
    for (int i =  0; i < NUM_PARA; i++) nums[i] = i;
    for (int i = 0; i < NUM_PARA; i++) {
        init_fifo(&queues[i]);
        pthread_create(&workers[i], NULL, &parallel_pairwise_comparator, &nums[i]);
    }

    pthread_t threads[size];
    memset(threads, 0, size * sizeof(pthread_t));

    hmsize_t max = size;
  
    printf("Creating threads for each cluster...\n");
    for (hmsize_t i = 0; i < size; ++i)
    {
        pthread_create(&threads[i], NULL, &hamming_cluster_single, &non_zero_clusters[i]);
    }

    printf("Joining threads...\n");
    for (hmsize_t i = 0; i < size; i++) {
        pthread_join(threads[i], NULL);
        printf("Thread %d with size %d joined\n", i, non_zero_clusters[i].n);
    }

    pthread_mutex_destroy(&global_lock);
    for (int i = 0; i < NUM_PARA; i++) {
        join_fifo(&queues[i]);
        pthread_join(workers[i], NULL);
        printf("Worker thread %d joined, lock & cond destoryed\n", i);
    }
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
    cluster_s curr_cluster;
    cluster_s *ret = calloc(1, 1);
    size_t non_zero = 0;

    for (tuphash_t i = 0; i < size; i++)
    {
        curr_cluster = clusters[i];

        if (curr_cluster.n < 2)
            continue;
        
        int s = curr_cluster.arr[0].out_len;
        int ss = curr_cluster.arr[1].out_len;

        non_zero++;
        ret = realloc(ret, non_zero * sizeof(cluster_s));
        ret[non_zero - 1] = curr_cluster;
    }
    printf("Got %lu non-zero clusters\n", non_zero);

    return (non_zero_clusters_s){.clusters = ret, .size = non_zero};
}

void cluster_ham_and_mark(chartype_t **seqs, size_t num_seqs, int k, int out[])
{
    global_out = out;
    printf("Clustering...\n");
    hm_s *clustered = cluster_seqs(seqs, num_seqs, K);
    printf("Done, getting non-szero clusters...\n");
    non_zero_clusters_s non_zeroes = filter_out_zero_clusters(clustered->vec_vec, clustered->n);
    printf("Doing hamming...\n");
    hamming_clusters_hm(non_zeroes.clusters, non_zeroes.size);
   // printf("Marking the results...\n");
   // mark_out(non_zeroes.clusters, non_zeroes.size, out);

    printf("Fully done!\n");
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
    self->vec_vec[index] = (cluster_s){.arr = (clusterseq_s*)calloc(1, sizeof(clusterseq_s)), .len_seq = len_seq, .n = 0x00000000, .hash = index + 1};
}
void resize_insert_hmn(clusterseqarr_t self, hmsize_t index, seq_t seq_packed, size_t out_len, size_t index_in_array)
{   
    self[index - 1] = (clusterseq_s){.dupes = (struct HashMapNode**)malloc(sizeof(struct HashMapNode*)), .seq_packed = seq_packed, .out_len = out_len, .size_dup = 0, .index_in_array = index_in_array, .is_dup = 0};
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

void init_fifo(fifo_s *self) {
    self->curr_index = self->has_member = self->join = 0;   
    pthread_mutex_init(&self->lock, NULL);
}

void put_fifo(fifo_s *self, pairwise_s item) {
    self->arr[self->curr_index] = item;

    if (self->curr_index++ > QBUFF - 1) {
        self->curr_index = QBUFF - 1;
    }

    self->has_member = 1;
}

pairwise_s pop_fifo(fifo_s *self) {
    if (--self->curr_index < 0) {
        self->curr_index = 0;
        self->has_member = 0;;
    }

    pairwise_s ret_pair = self->arr[self->curr_index];   
    return ret_pair;
}

void lock_fifo(fifo_s *self) {
    pthread_mutex_lock(&self->lock);
}

void unlock_fifo(fifo_s *self) {
    pthread_mutex_unlock(&self->lock);
}

void join_fifo(fifo_s *self) {
    while (1) {
        if (self->has_member) {
            continue;
        } else {
            self->join = 1;
            break;
        }
    }
}

