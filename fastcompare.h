#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DELIM '|'
const char NULLCHR = '\0';

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

int pad_and_get_lazy_hamming(char *s1, char *s2);
void find_hammings_and_mark(char *in_pipe_separated, int outs_labels[]);
void str_split(str_split_s *self, char *in);
char *tokenizer(char *in, int *head, char *fragment);