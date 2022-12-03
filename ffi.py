import cffi
import os

ffi = cffi.FFI()

ffi.cdef("""
    typedef uint8_t chartype_t;
    typedef uint64_t outtype_t;
    typedef uint64_t hamtype_t;

    #define SIZE_CHARS 32
    #define SIZE_OUT 4
    #define SIZE_HAM 4

    void get_hamming_integers(hamtype_t a[SIZE_HAM], hamtype_t b[SIZE_HAM], int outs_label[1]);
    void reduce_integer_or_op(outtype_t in, outtype_t *reducer);
    void encode_gatacca(chartype_t in[SIZE_CHARS], outtype_t out[SIZE_OUT]);
    void pack_32_bytes_in_64_bits(chartype_t in[SIZE_CHARS], outtype_t result[1]);
    """)

ffi.set_source("_fastcompare",
    '#include "/usr/local/include/fastcompare.h"',
    libraries=["fastcompare"],
    library_dirs=[os.path.dirname(__file__), "/usr/local/lib/"],
)

ffi.compile()
