#define K 4
#define NUM_PARA 48
#define QBUFF   2048

typedef struct PairWiseSeq {
    clusterseq_s *lead;
    clusterseq_s *candidate;
    int *skip_rest;
} pairwise_s;


typedef struct FifOQueue {
    pairwise_s arr[QBUFF];
    int curr_index;
    int has_member;
    int join;
    pthread_mutex_t lock;
} fifo_s;

void init_fifo(fifo_s *self);
void put_fifo(fifo_s *self, pairwise_s item);
pairwise_s pop_fifo(fifo_s *self);
void lock_fifo(fifo_s *self);
void unlock_fifo(fifo_s *self);
void join_fifo(fifo_s *self);