#include "../include/fastcompare.h"

void init_fifo(fifo_s *self) {
    self->curr_index = self->engaged = self->curr_head = self->has_member = self->join = 0;   
    self->arr = NULL;
    self->started = time(NULL);
}

void put_fifo(fifo_s *self, clusterseq_s *lead, clusterseq_s *candidate, int index_in_cluster, int *skip_rest) {
    lock_fifo(self);
    
    int old_size = self->curr_index * sizeof(pairwise_s);
    int new_size = ++self->curr_index * sizeof(pairwise_s);

    pairwise_s *nnptr = (pairwise_s *)realloc_zero(self->arr, old_size, new_size);

    if (!nnptr) {
        printf("Error reallocating queeu.\n");
        exit(ENOMEM);
    }

    self->arr = nnptr;
    self->arr[self->curr_index - 1] = (pairwise_s){
        .lead=lead,
        .candidate=candidate,
        .index_in_cluster=index_in_cluster,
        .skip_rest=skip_rest,
    };
    self->has_member = 1;

    unlock_fifo(self);
}

pairwise_s pop_fifo(fifo_s *self) {
    lock_fifo(self);

    if (self->curr_index - self->curr_head < 0) {
        self->has_member = 0;
        self->join = 1;

        return (pairwise_s){
            .lead=0x00, 
            .candidate=0x00, 
            .index_in_cluster=-1, 
            .skip_rest=0x00
        };
    }

    pairwise_s ret_pair = self->arr[self->curr_head++];       
    unlock_fifo(self);
    
    return ret_pair;
}

void lock_fifo(fifo_s *self) {
    pthread_mutex_lock(&self->lock);
}

void unlock_fifo(fifo_s *self) {
    pthread_mutex_unlock(&self->lock);
}

pairwise_s create_pair(clusterseq_s *arr, int i, int j, int *skip) {
    return (pairwise_s){.lead=&arr[i], .candidate=&arr[j], .skip_rest=skip};
}


