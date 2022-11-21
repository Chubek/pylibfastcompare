from cffi import FFI
ffibuilder = FFI()


ffibuilder.cdef(
    """
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
    
    int get_lazy_hamming(char *s1, char *s2, int num_cols);
    void find_hammings_and_mark(const unsigned char **in_variadic_subchar, int outs_labels[], int len_rows, int len_cols);
    void str_split(str_split_s *self, char *in);
    char *tokenizer(char *in, int *head);
   """
)

ffibuilder.set_source("_fastcompare", 
    '#include "fastcompare.h"',
    sources=['fastcompare.c'], 
    libraries=["c"],
)  

if __name__ == "__main__":
    ffibuilder.compile(verbose=True)