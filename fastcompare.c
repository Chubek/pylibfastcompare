#include "fastcompare.h"



/*
given a null-terminaed, pipe-separated buffer of chars, it will first separate them,
then run through the separated strings in a double loop with a skip of
one in the second loop. If that number has been eliminated, it will skip.
*/
void find_hammings_and_mark(char *in_pipe_separated, int outs_labels[]) {
    str_split_s split_s;
    str_split(&split_s, in_pipe_separated);
    int diff = 0;

    for (size_t i = 0; i < split_s.cnt; ++i) {        
        if (outs_labels[i] != 0) continue;

        for (size_t j = i + 1; j < split_s.cnt; ++j) {
            if (outs_labels[j] != 0) continue;

            char *s1 = split_s.splt_strs[i];
            char *s2 = split_s.splt_strs[j];
            diff = pad_and_get_lazy_hamming(s1, s2);;
            outs_labels[j] = diff;
        }
    }

    free(split_s.splt_strs);    
}

/*
pads the smaller string to the size of the larger string and 
gets their lazy hamming
*/
int pad_and_get_lazy_hamming(char *s1, char *s2) {
    int diff = 0;

    str_size_s str_lens = ARG_MAX_STR(s1, s2);
    
    char c_s = *str_lens.smaller_str++;
    char c_l = *str_lens.larger_str++;

    while (c_s) {
        if (c_s == c_l) ++diff;
        if (diff > 1) return 0;

        c_s = *str_lens.smaller_str++;
        c_l = *str_lens.larger_str++;
    }   

    return diff;    
}

/*
tokenize str on DELIM
*/
char *tokenizer(char *in, int *head, char *fragment) {
    in = &in[*head];
    if (in[0] == DELIM) *in++;

    int size_token = 1;
    char c;
    do {
        fragment  = realloc(fragment, size_token);
        
        c = *in++;
        (*head)++;
        if (c == DELIM) {
            fragment = realloc(fragment, size_token);
            strncat(fragment, &NULLCHR, 1);
            break;
        }
        ++size_token;
        strncat(fragment, &c, 1);
    } while(1);
}

/*
split str on DELIM
*/
void str_split(str_split_s *self, char *in) {
    char *in_cpy = malloc(sizeof(in));
    size_t in_len = strlen(in);
    strncat(in_cpy, in, in_len);
    int cnt_pipes = 0;
    char c;

    while((c = *in_cpy++)) {
        if (c == DELIM) cnt_pipes++;
    }
    
    char **out = malloc(cnt_pipes * sizeof(char*));
    int head = 0;
    char *fragment = malloc(sizeof(char));

    int i = 0;
    while(1) {
        fragment = realloc(fragment, sizeof(char));
        memset(fragment, 0, 1);

        tokenizer(in, &head, fragment);
        size_t size_of_handle = strlen(fragment) + 1;
        out[i] = realloc(out[i], size_of_handle);
        
        for (size_t j = 0; j < size_of_handle; ++j) {
            *(out[i] + j) = *(fragment + j);
        }
        
        if (head == in_len) break;

        ++i;
    }

    free(fragment);

    self->splt_strs = out;
    self->cnt = cnt_pipes;
}



