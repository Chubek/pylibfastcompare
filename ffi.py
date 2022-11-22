import cffi
import os

ffi = cffi.FFI()

ffi.cdef("""
    void find_hammings_and_mark(uint8_t char *in[], int outs_labels[], size_t len_rows, size_t maxlen);
    """)

ffi.set_source("_fastcompare",
    '#include "fastcompare.h"',
    libraries=["fastcompare"],
    library_dirs=[os.path.dirname(__file__),],
)

ffi.compile()
