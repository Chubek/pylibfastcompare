import cffi
import os

ffi = cffi.FFI()

ffi.cdef("""
    void iterate_and_eliminate(const unsigned char *ls, int num_rows, int num_cols, int out[]);
""")

ffi.set_source("_fastcompare",
    '#include "fastcompare.h"',
    libraries=["fastcompare"],
    library_dirs=[os.path.dirname(__file__),],
)

ffi.compile()
