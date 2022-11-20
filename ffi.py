import cffi
import os

ffi = cffi.FFI()

ffi.cdef("""
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
    
    int get_lazy_hamming(char *s1, char *s2);
    void find_hammings_and_mark(char *in_pipe_separated, int outs_labels[]);
    void str_split(str_split_s *self, char *in);
    char *tokenizer(char *in, int *head, char *fragment);
""")

ffi.set_source("_fastcompare",
    '#include "fastcompare.h"',
    libraries=["fastcompare"],
    library_dirs=[os.path.dirname(__file__),],
)

ffi.compile()
