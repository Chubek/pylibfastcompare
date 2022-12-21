#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c\n"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


void quicksort(chartype_t *in, int curr_l, int curr_h);
int quicksort_partition(chartype_t *in, int curr_l, int curr_h);
void swap(chartype_t *in, int i, int j);
int sleepms(long miliseconds);
int max_len(int *lens, int size);