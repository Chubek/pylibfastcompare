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