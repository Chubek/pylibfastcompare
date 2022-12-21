import cffi
import os

ffi = cffi.FFI()

ffi.cdef("""   
    typedef uint8_t chartype_t;

    void cluster_ham_and_mark(chartype_t *seqs, int *len_seqs, size_t num_seqs, int k, int out[]);
    """, override=True)

ffi.set_source("_fastcompare",
    '#include "/usr/local/include/fastcompare/fastcompare.h"',
    libraries=["fastcompare"],
    library_dirs=[os.path.dirname(__file__), "/usr/local/lib/"],
)

ffi.compile()
