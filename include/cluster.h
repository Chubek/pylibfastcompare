typedef struct NonZeroCluster {
    clusterseq_s *clusterseq_arr;
    uint32_t n;
} non_zero_cluster_s;

typedef struct NonZeroClusters {
    non_zero_cluster_s *nz_clusters;
    tuphash_t size;
} non_zero_clusters_s;

typedef struct NonZeroClusterCluster {
    non_zero_cluster_s *clusters;
    int start;
    int end;
} cluster_cluster_s;

#define SZ_NZC sizeof(non_zero_cluster_s)
#define SZ_NZS sizeof(non_zero_clusters_s)

#define SZ_CLST sizeof(cluster_s)

non_zero_clusters_s filter_out_zero_clusters(hm_s *hm);
hm_s cluster_seqs(chartype_t **seqs, size_t num_seqs, int k);
void insert_seq_in_hm(hm_s *self, chartype_t *seq, size_t index_in_array, int k);
uint64_t get_kmer_key(seq_t out, int size_out, int k);
uint64_t merge_freqs(int freqs[256]);
uint64_t murmurhash(const uint64_t key, int non_zero, uint32_t seed);
void get_freq_value(chartype_t *in, int size_in, int uint8_freqs[256]);
void get_kmers(chartype_t *in, chartype_t out[], int size, int k);
void and_buffer(chartype_t buffer[32], chartype_t out[], int starting_point, int size, int k);
void encode_to_0123(chartype_t in[32], chartype_t out[32]);
chartype_t pack_1_byte_into_8_bits(chartype_t enc1, chartype_t enc2, chartype_t enc3, chartype_t enc4);