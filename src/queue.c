#include "../include/fastcompare.h"

#define NUM_PARA 48

void init_fifo(fifo_s *self) {
    self->arr = NULL;
    self->curr_index = self->has_member = self->join = 0;   
    pthread_mutex_init(&self->lock, NULL);
}

void put_fifo(fifo_s *self, pairwise_s item) {
    lock_fifo(self);

    fifo_s *cow = calloc(1, sizeof(fifo_s));
    *cow = *self;

    size_t old_size = cow->curr_index * SZ_PAIR;
    size_t new_size = ++cow->curr_index * SZ_PAIR;

    pairwise_s *nnptr = (pairwise_s *)realloc_zero(cow->arr, old_size, new_size);

    if (!nnptr) {
        printf("Error reallocating queue.\n");
        exit(ENOMEM);
    }

    cow->arr = nnptr;
    cow->arr[cow->curr_index - 1] = item; 
    cow->has_member = 1;

    *self = *cow;

    free(cow);

    unlock_fifo(self);
}

pairwise_s pop_fifo(fifo_s *self) {
    lock_fifo(self);

    if (--self->curr_index < 0) {
        self->curr_index = 0;
        self->join = 1;
    }

    pairwise_s ret_pair = self->arr[self->curr_index];   
    
    unlock_fifo(self);
    
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

pairwise_s create_pair(clusterseq_s *arr, int i, int j, int *skip) {
    return (pairwise_s){.lead=&arr[i], .candidate=&arr[j], .skip_rest=skip};
}