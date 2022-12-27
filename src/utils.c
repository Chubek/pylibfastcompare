#include "../include/fastcompare.h"


int sleepms(long miliseconds)
{
   struct timespec rem;
   struct timespec req= {
       (int)(miliseconds / 1000),     /* secs (Must be Non-Negative) */ 
       (miliseconds % 1000) * 1000000 /* nano (Must be in range of 0 to 999999999) */ 
   };

   return nanosleep(&req , &rem);
}

void sleepns_portable(unsigned long nsec) {
    struct timespec delay = { nsec / 1000000000, nsec % 1000000000 };
    pselect(0, NULL, NULL, NULL, &delay, NULL);
}


void swap(chartype_t *in, int i, int j)
{
    chartype_t tmp = in[i];
    in[i] = in[j];
    in[j] = tmp;
}

int quicksort_partition(chartype_t *in, int curr_l, int curr_h)
{
    chartype_t pivot = in[curr_h];

    int i = curr_l - 1;
    for (int j = curr_l; j < curr_h; j++)
    {
        if (in[j] < pivot)
        {
            i++;
            swap(in, i, j);
        }
    }

    swap(in, i + 1, curr_h);
    return i + 1;
}

void quicksort(chartype_t *in, int curr_l, int curr_h)
{
    if (curr_l < curr_h)
    {
        int partition_index = quicksort_partition(in, curr_l, curr_h);
        quicksort(in, curr_l, partition_index - 1);
        quicksort(in, partition_index + 1, curr_h);
    }
}


int max_len(int *lens, int size) {
    int max = 0;

    for (int i = 0; i < size; i++) {
        if (lens[i] > max) {
            max = lens[i];
        }
    }

    return max;
}

void* realloc_zero(void* p_fifo, size_t old_size, size_t new_size) {
  void* p_new = realloc(p_fifo, new_size);
  if ( new_size > old_size && p_new ) {
    size_t diff = new_size - old_size;
    void* p_start = ((char*)p_new) + old_size;
    memset(p_start, 0, diff);
  }
  return p_new;
}

int sum_array(int in[], int size) {
    int out = 0;

    for (int i = 0; i < size; i++) out += in[i];

    return out;
}