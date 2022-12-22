#include "../include/fastcompare.h"


non_zero_clusters_s filter_out_zero_clusters(hm_s *hm)
{      
    int size_clusters = 0;
    non_zero_clusters_s nz_clusters = (non_zero_clusters_s){.nz_clusters=NULL, .size=0};
    
    bucket_s curr_bucket;
    cluster_s curr_cluster;

    for (tuphash_t i = 0; i < HASH_MAX; i++) {
        if (hm->occupied[i] != OCCUPIED) continue;

        curr_bucket = hm->bucket_arr[i];
        if (curr_bucket.n == 2) continue;

        non_zero_cluster_s curr_nz = (non_zero_cluster_s){.clusterseq_arr=NULL, .n=0};
        uint32_t curr_size = 0;  

        for (tuphash_t j = 0; j < BUCKET_HASH_MAX; j++) {
            if (curr_bucket.occuped[j] != OCCUPIED) continue;
            
            curr_cluster = curr_bucket.cluster_arr[j];
            if (curr_cluster.n < 2) continue;         

            curr_size += curr_cluster.n;

           clusterseq_s *nnptr = (clusterseq_s *)realloc(curr_nz.clusterseq_arr, curr_size * SZ_CLSQ);

            if (!nnptr) {
                printf("Error reallocating NZ cluster.\n");
                exit(ENOMEM);
            }
            
            curr_nz.clusterseq_arr = nnptr;
            
            int j = 0;
            for (int i = curr_nz.n; i < curr_size; i++) {
                curr_nz.clusterseq_arr[i] = curr_cluster.clusterseq_arr[j++];
            }

            curr_nz.n = curr_size;
        }

        if (curr_size == 0) continue;

        non_zero_cluster_s *nnptr = (non_zero_cluster_s *)realloc(nz_clusters.nz_clusters, ++size_clusters * SZ_NZC);

        if (!nnptr) {
                printf("Error reallocating NZ clusters.\n");
                exit(ENOMEM);
        }


        nz_clusters.nz_clusters = nnptr;
        nz_clusters.nz_clusters[size_clusters - 1] = curr_nz;
    }

    nz_clusters.size = size_clusters;


    printf("Got %d non-zero clusters\n", size_clusters);
    return nz_clusters;
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
    int max_len = 0;
    int diffed_len = 0;

    for (int i = 0; i < size; i += SIZE_CHARS)
    {
        memset(buffer, 0, 32);

        max_len = size - diffed_len >= SIZE_CHARS ? SIZE_CHARS : size - diffed_len;

        for (int j = 0; j < max_len; j++) buffer[j] = in[j + i];

        and_buffer(buffer, out, i, size, k);

        diffed_len += SIZE_CHARS;
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

void insert_seq_in_hm(hm_s *self, chartype_t *seq, size_t index_in_array, int k)
{
    int len_seq = strlen(seq);
    int size_out = (len_seq / k);
    seq_t out = (seq_t)calloc(size_out + 1, sizeof(chartype_t));

    get_kmers(seq, out, len_seq, k);
    uint64_t key = get_kmer_key(out, size_out, k);
    uint64_t djb = djb2(seq, len_seq);

    key ^= djb >> 48;

    insert_seq_into_hashmap(self, key, out, len_seq, size_out, index_in_array);
}

hm_s cluster_seqs(chartype_t **seqs, size_t num_seqs, int k)
{
    hm_s hm = new_hashmap();
    
    for (int i = 0; i < num_seqs; i++)
    {
        insert_seq_in_hm(&hm, seqs[i], i, k);
    }

    return hm;
}
