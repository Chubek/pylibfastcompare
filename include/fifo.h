#define K 4
#define NUM_PARA 256
#define PARA_DIV 10000
#define QBUFF   4096

typedef struct PairWiseSeq {
    clusterseq_s *lead;
    clusterseq_s *candidate;
    int index_in_cluster;
    int *skip_rest;
} pairwise_s;

#define SZ_PAIR sizeof(pairwise_s)

typedef struct FifoQueue {
    pairwise_s *arr;
    pthread_mutex_t lock;
    int join, curr_index, curr_head, has_member, engaged;
    time_t started;
} fifo_s;

void init_fifo(fifo_s *self);
void put_fifo(fifo_s *self, clusterseq_s *lead, clusterseq_s *candidate, int index_in_cluster, int *skip_rest);
pairwise_s pop_fifo(fifo_s *self);
void lock_fifo(fifo_s *self);
void unlock_fifo(fifo_s *self);
pairwise_s create_pair(clusterseq_s *arr, int i, int j, int *skip);
void get_set_all_popped_num(fifo_s *self);