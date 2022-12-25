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
fifo_s *queues;
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
int size_arr = 0;
int timeout = 0;
int engagd_workers[NUM_PARA];

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

void _hamming_clusters_hm(non_zero_cluster_s *non_zero_clusters, tuphash_t size)
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



void *_hamming_cluster_single(void *cluster_ptr)
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

    hamtype_t a_fifo[SIZE_HAM];
    hamtype_t b_fifo[SIZE_HAM];
    size_t num_bytes = SIZE_HAM * sizeof(hamtype_t);    

    seq_t a_seq = a.seq_packed;
    seq_t b_seq = b.seq_packed;
    int out_len = a.out_len;

    int max_len = 32;
    int diffed_len = 0;

    for (size_t i = 0; i < out_len; i += SIZE_HAM)
    {

        memset(a_fifo, 0, num_bytes);
        memset(b_fifo, 0, num_bytes);        
        
        max_len = out_len - diffed_len >= SIZE_HAM ? SIZE_HAM : out_len - diffed_len;

        for (int j = 0; j < max_len; j++) a_fifo[j] = a_seq[i + j];
        for (int j = 0; j < max_len; j++) b_fifo[j] = b_seq[i + j];

        new_diff = get_hamming_integers(a_fifo, b_fifo);
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

/* seq must be null-terminated */
void hamming_clusters_hm(non_zero_cluster_s *non_zero_clusters, tuphash_t size)
{
    if (pthread_mutex_init(&global_lock, NULL) != 0) {
        printf("Failed to initiailize global thread lock. Exiting...\n");
        exit(1);
    }

    queues = calloc(NUM_PARA, sizeof(fifo_s));

    pthread_t workers[NUM_PARA];
    int nums[NUM_PARA];
    for (int i = 0; i < NUM_PARA; i++) {
        nums[i] = i;
        init_fifo(&queues[i]);
        pthread_create(&workers[i], NULL, &parallel_pairwise_comparator, &nums[i]);
    }

    int max = NUM_PARA;

    printf("%d workers started...", NUM_PARA);
    int ms = 0;
    printf("Creating threads for each cluster...\n");
    if (size >= PARA_DIV) {
        cluster_cluster_s *cluster_clusters = calloc(size / (PARA_DIV), sizeof(cluster_cluster_s));

        for (int i = 0; i < NUM_PARA; i += PARA_DIV) {
            cluster_clusters[i].clusters = non_zero_clusters;
            cluster_clusters[i].start = i;
            cluster_clusters[i].end = i + (PARA_DIV);
        }

        printf("Starting %d threads...\n", size / (PARA_DIV));
        pthread_t threads[size / (PARA_DIV)];
        memset(threads, 0, (size / (PARA_DIV)) * sizeof(pthread_t));

        for (hmsize_t i = 0; i < size / (PARA_DIV); ++i)
        {
            pthread_create(&threads[i], NULL, &hamming_cluster_list, &cluster_clusters[i]);
        }

        printf("Joining threads...\n");
        for (hmsize_t i = 0; i < size / (PARA_DIV); i++) {
            pthread_join(threads[i], NULL);
        }

        ms = (size / (PARA_DIV)) * 1;

    } else {
        printf("Starting %d threads...\n", size);


        pthread_t threads[size];
        memset(threads, 0, (size) * sizeof(pthread_t));

        for (hmsize_t i = 0; i < size; ++i)
        {
            pthread_create(&threads[i], NULL, &hamming_cluster_single, &non_zero_clusters[i]);
        }

        printf("Joining threads...\n");
        for (hmsize_t i = 0; i < size; i++) {
            pthread_join(threads[i], NULL);
        }

        timeout = 1;
        max = size;

        ms = size * 150;
    }        

    pthread_mutex_destroy(&global_lock);

   
    printf("Waiting for %d millis for workers to engage...\n", ms);
    sleepms(ms);

    int num_joined = 0;

    printf("Workers joined: ");

    while (1) {        
        if (num_joined == sum_array(engagd_workers, NUM_PARA)) break;

        for (int i = 0; i < NUM_PARA; i++) {
            if (nums[i] == -1) continue;
            if (engagd_workers[i] == 0) continue;
            
            if (queues[i].join == 1) {
                pthread_join(workers[i], NULL);
                printf("%d... ", i);
                num_joined += 1;
                nums[i] = -1;
            }
        }      

    }

    printf("\n");
}



void *hamming_cluster_list(void *cluster_ptr)
{
    cluster_cluster_s *clusters = (cluster_cluster_s *)cluster_ptr;

    for (int i = clusters->start; i < clusters->end; i++) {
        non_zero_cluster_s cluster = clusters->clusters[i];

        if (!cluster.clusterseq_arr) continue;

        clusterseqarr_t cluster_seqs = cluster.clusterseq_arr;
        hmsize_t cluster_size = cluster.n;
        
        anew:
        clusterseq_s *lead;
        clusterseq_s *candidate;
        int k = 0;
        int skip_rest[cluster_size];
        memset(skip_rest, 0, 4 * cluster_size);
        int sum = 0;

        for (int i = 0; i < cluster_size; i++) {
            set_skip:
            if (sum_array(skip_rest, cluster_size) == cluster_size) goto anew;

            lead = &cluster_seqs[i];
           
            if (lead->index_in_array > size_arr) continue;
            if (lead->is_dup == 1) continue;

            for (int j = i + 1; j < cluster_size; j++) {
                if (skip_rest[j] == 1) goto set_skip;

                candidate = &cluster_seqs[j];

                if (candidate->index_in_array > size_arr) continue;
                if (candidate->is_dup == 1) continue;

                if (k == NUM_PARA) k = 0;

                put_fifo(&queues[k++], (pairwise_s){
                    .lead=lead,
                    .candidate=candidate,
                    .index_in_cluster=j,
                    .skip_rest=skip_rest
                });
           }
        }
    }    
}

void *hamming_cluster_single(void *cluster_ptr)
{
    non_zero_cluster_s *cluster = (non_zero_cluster_s *)cluster_ptr;

    clusterseqarr_t cluster_seqs = cluster->clusterseq_arr;
    hmsize_t cluster_size = cluster->n;

    int diff = 0;

    clusterseq_s *lead;
    clusterseq_s *candidate;
    int k = 0;
    int skip_rest[cluster_size];
    memset(skip_rest, 0, 4 * cluster_size);


    for (int i = 0; i < cluster_size; i++) {
        set_skip:
        if (skip_rest[i] == 1) continue;

        lead = &cluster_seqs[i];

        if (!lead) continue;;
        if (lead->out_len == 0 | lead->seq_packed == NULL) continue;;
        if (lead->index_in_array > size_arr) continue;
        if (lead->is_dup == 1) continue;

        for (int j = i + 1; j < cluster_size; j++) {
            if (skip_rest[j] == 1)  goto set_skip;

            candidate = &cluster_seqs[j];

            if (!candidate) continue;;
            if (candidate->out_len == 0 | candidate->seq_packed == NULL) continue;;
            if (candidate->index_in_array > size_arr) continue;
            if (candidate->is_dup == 1) continue;

            if (k == NUM_PARA) k = 0;

            put_fifo(&queues[k++], (pairwise_s){
                .lead=lead,
                .candidate=candidate,
                .index_in_cluster=j,
                .skip_rest=skip_rest
            });
        }
    }
}

void *parallel_pairwise_comparator(void *num) {
    int *np = (int*)num;
    int n = *np;
    int diff;
    pairwise_s curr_pair;  
    fifo_s *q = &queues[n];
    time_t t0 = time(NULL);
    time_t t1 = time(NULL);
    time_t dt;

    do {
        if (q->has_member == 1) {
            curr_pair = pop_fifo(q);

            pthread_mutex_lock(&global_lock);
            engagd_workers[n] = 1;
            pthread_mutex_unlock(&global_lock);

            
            if (curr_pair.lead->is_dup == 1) continue;
            if (curr_pair.candidate->is_dup == 1) continue;
            if (curr_pair.skip_rest[curr_pair.index_in_cluster] == 1) continue;

            int diff = hamming_hseq_pair(*curr_pair.lead, *curr_pair.candidate);
            if (diff < 2) {
                pthread_mutex_lock(&global_lock);
                global_out[curr_pair.candidate->index_in_array] = curr_pair.lead->index_in_array;
                pthread_mutex_unlock(&global_lock);
                curr_pair.skip_rest[curr_pair.index_in_cluster] = 1;
                curr_pair.candidate->is_dup = 1;
            }

        }

        if (timeout == 1) {
            t1 = time(NULL);
            dt = t1 - t0;

            if (dt >= MAX_TIMEOUT) {
                break;
            }
        }


    } while(!q->join);

}


