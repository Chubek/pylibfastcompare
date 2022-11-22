from cffi import FFI
ffibuilder = FFI()

ffibuilder.cdef(
    f"""
    void find_hammings_and_mark(char *in[], int outs_labels[], int len_rows, int maxlen);
    """
)

ffibuilder.set_source("_fastcompare", 
    '#include "fastcompare.h"',
    sources=['fastcompare.c'], 
    libraries=["c"],
)  

if __name__ == "__main__":
    ffibuilder.compile(verbose=True)