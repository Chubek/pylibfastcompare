#include "../include/fastcompare.h"

void init_fifo(fifo_s *self) {
    self->arr = NULL;
    self->curr_index = self->has_member = self->join = 0;   
    pthread_mutex_init(&self->lock, NULL);
}

void put_fifo(fifo_s *self, clusterseq_s *lead, clusterseq_s *candidate, int *skip_rest) {
    lock_fifo(self);
    int index = self->curr_index++ % QBUFF;
    self->arr[index].lead = lead;
    self->arr[index].candidate = candidate;
    self->arr[index].skip_rest = skip_rest;
    self->arr[index].popped = 0;    
    unlock_fifo(self);
}

pairwise_s pop_fifo(fifo_s *self) {
    lock_fifo(self);
    pairwise_s ret_pair = self->arr[(QBUFF - (self->curr_index % QBUFF)) - 1];       
    self->arr[(QBUFF - (self->curr_index % QBUFF)) - 1].popped = 1;
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