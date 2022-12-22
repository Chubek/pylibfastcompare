#include "../include/fastcompare.h"

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
int size_arr = 0;

void cluster_ham_and_mark(chartype_t **seqs, size_t num_seqs, int k, int out[])
{   
    global_out = out;
    size_arr = num_seqs;

    printf("Clustering...\n");    
    hm_s clustered = cluster_seqs(seqs, num_seqs, K);

    printf("Done, getting non-szero and non-one clusters...\n");
    non_zero_clusters_s non_zeroes = filter_out_zero_clusters(&clustered);
    
    printf("Doing hamming...\n");
    hamming_clusters_hm(non_zeroes.nz_clusters, non_zeroes.size); 

    printf("Fully done!\n");
}

void hamming_clusters_hm(non_zero_cluster_s *non_zero_clusters, tuphash_t size)
{
    if (pthread_mutex_init(&global_lock, NULL) != 0) {
        printf("Failed to initiailize global thread lock. Exiting...\n");
        exit(1);
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
    }

    pthread_mutex_destroy(&global_lock);
}



void *hamming_cluster_single(void *cluster_ptr)
{
    non_zero_cluster_s *cluster = (non_zero_cluster_s *)cluster_ptr;

    clusterseqarr_t cluster_seqs = cluster->clusterseq_arr;
    hmsize_t cluster_size = cluster->n;

    int diff = 0;

    clusterseq_s *lead = NULL;
    clusterseq_s *candidate = NULL;
    int i = 0;
    int ahead = 0;

    do
    {
        lead = &cluster_seqs[i];

        if (!lead) continue;
        if (!lead->seq_packed) continue;
        if (lead->index_in_array >= size_arr) continue;
        if (lead->is_dup == 1) continue;
       
        anew:   
        diff = 0;
        int j = i + 1 + ahead;

        if (j >= cluster_size) goto set_ahead;
        
        do 
        {
            candidate = &cluster_seqs[j];      

            if (!candidate) continue;
            if (!candidate->seq_packed) continue;
            if (candidate->index_in_array >= size_arr) continue;
            if (candidate->is_dup == 1) continue;

            diff = hamming_hseq_pair(*lead, *candidate);

            if (diff < 2) {
                pthread_mutex_lock(&global_lock);
                global_out[candidate->index_in_array] = lead->index_in_array;
                pthread_mutex_unlock(&global_lock);
                candidate->is_dup = 1;
                ahead++;
                goto anew;
            }
            
        } while (j++ < cluster_size);

        set_ahead:
        ahead = 0;
    } while (i++ < cluster_size); 
}


int hamming_hseq_pair(clusterseq_s a, clusterseq_s b)
{
    int diff = 0;
    int new_diff = 0;

    if (a.out_len != b.out_len) return 0;

    hamtype_t a_buffer[SIZE_HAM];
    hamtype_t b_buffer[SIZE_HAM];
    size_t num_bytes = SIZE_HAM * sizeof(hamtype_t);    

    seq_t a_seq = a.seq_packed;
    seq_t b_seq = b.seq_packed;
    int out_len = a.out_len;

    int max_len = 32;
    int diffed_len = 0;

    for (size_t i = 0; i < out_len; i += SIZE_HAM)
    {

        memset(a_buffer, 0, num_bytes);
        memset(b_buffer, 0, num_bytes);        
        
        max_len = out_len - diffed_len >= SIZE_HAM ? SIZE_HAM : out_len - diffed_len;

        for (int j = 0; j < max_len; j++) a_buffer[j] = a_seq[i + j];
        for (int j = 0; j < max_len; j++) b_buffer[j] = b_seq[i + j];

        new_diff = get_hamming_integers(a_buffer, b_buffer);
        diff += new_diff;

        diffed_len += SIZE_HAM;
    }
 
    return diff;
}

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
