#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DELIM '|'
#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))
#define NUM_ROWS(array_2d) ARRAY_LEN(array_2d)
#define NUM_COLS(array_2d) ARRAY_LEN(array_2d[0])

typedef struct {
    size_t larger_str_size;
    size_t smaller_str_size;
    char *smaller_str;
    char *larger_str;
} str_size_s;

typedef struct {
    char **splt_strs;
    int cnt;
} str_split_s;

/// Get argument max of two numbers
#define ARG_MAX_STR(s1, s2)  ((strlen(s1) >= strlen(s2)) ? (str_size_s){.larger_str_size=strlen(s1), .smaller_str_size=strlen(s2), .smaller_str=s2, .larger_str=s1} : (str_size_s){.larger_str_size=strlen(s2), .larger_str=s1, .smaller_str_size=strlen(s1), .larger_str=s2, .smaller_str=s1})

int get_lazy_hamming(char *s1, char *s2, int num_cols);
void find_hammings_and_mark(const unsigned char **in_variadic_subchar, int outs_labels[], int len_rows, int len_cols);
void str_split(str_split_s *self, char *in);
char *tokenizer(char *in, int *head);