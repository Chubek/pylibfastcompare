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
            diff = get_lazy_hamming(s1, s2);;
            outs_labels[j] = diff;
        }
    }

    free(split_s.splt_strs);    
}

/*
pads the smaller string to the size of the larger string and 
gets their lazy hamming
*/
int get_lazy_hamming(char *s1, char *s2) {
    size_t diff = 0;

    str_size_s str_lens = ARG_MAX_STR(s1, s2);
    
    char c_s = *str_lens.smaller_str++;
    char c_l = *str_lens.larger_str++;

    while (c_s) {
        if (c_s != c_l) ++diff;
        if (diff > 1) return 0;

        c_s = *str_lens.smaller_str++;
        c_l = *str_lens.larger_str++;
    }   

    return 1;    
}

/*
tokenize str on DELIM
*/
char *tokenizer(char *in, int *head) {
    in = &in[*head];
    if (in[0] == DELIM) *in++;
    const char NULLCHR = '\0';
    const size_t BUFFSIZE = 64;

    int size_token = 0;
    int m = 0;
    int prog = 0;
    size_t new_size = 0;
    size_t old_size = 0;
    int len_buffer = 0;
    char buffer[BUFFSIZE];
    memset(buffer, 0, BUFFSIZE);
    char *frag = calloc(1, 1);
    char *new_alloc;

    char c;
    while((c = *in++)) {   
        (*head)++;
        if (c == DELIM) {
            new_size += size_token;

            new_alloc = realloc(frag, new_size + 1);
            if (!new_alloc) {
                printf("Error doing the final token realloc, exiting...\n");
                exit(1);
            } else {
                frag = new_alloc;
            }

            m = 0;
            for (size_t k = old_size; k < new_size; ++k) {
                *(frag + k) =  buffer[m++];
            }
            frag[new_size] = NULLCHR;

            break;
        }
        
        if (size_token == BUFFSIZE) {
            new_size = BUFFSIZE * ++prog;
            new_alloc = realloc(frag, new_size);

            if (!new_alloc) {
                printf("Error reallocating token fragment, exiting...\n");
                exit(1);
            } else {
                frag = new_alloc;
            }

            m = 0;
            for (size_t k = old_size; k < new_size; ++k) {
                *(frag + k) =  buffer[m++];
            }
            old_size = new_size;
            size_token = 0;
            memset(buffer, 0, BUFFSIZE);
        } else {
            buffer[size_token] = c;      
            ++size_token;
        }

    }

    return frag;
}

/*
split str on DELIM
*/
void str_split(str_split_s *self, char *in) {
    size_t in_len = strlen(in);
    int cnt_pipes = 0;

    for(size_t i = 0; i < in_len; ++i) {
        if (in[i] == DELIM) cnt_pipes++;
    }

    char **out = malloc(cnt_pipes);
    for (size_t m = 0; m < cnt_pipes; ++m) {
        out[m] = calloc(1, 1);
    }

    int head = 0;
    char *fragment;
    char *new_alloc;

    int i = 0;
    while(1) {
        fragment = tokenizer(in, &head);        
        out[i] = strdup(fragment);
        if (head == in_len) break;
        ++i;
    }


    free(fragment);
    
    self->splt_strs = out;
    self->cnt = cnt_pipes;
}

