#include "../include/fastcompare.h"

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

