import json
import sys
from pathlib import Path
from queue import Queue
from threading import Thread
from typing import Any, Dict, List, Tuple

import cffi
from Bio.SeqIO.FastaIO import SimpleFastaParser

from _fastcompare import lib
from mmapfastaparser import MmapFastaParser

ffi = cffi.FFI()

def seqs_bytes(
    path: str,
) -> Tuple[List[bytearray], int]:
    list_bytes = []
    i = 0
    for header, seq in SimpleFastaParser(open(path, 'r')):
        list_bytes.append(ffi.new("char[]", (seq.encode('ascii') + '\0'.encode('ascii'))))
        i += 1

    return list_bytes, i


def run_libfastcompare(path: str):
    print('Reading, getting bytes...')
    bytes_, size = seqs_bytes(path)
    print(f"Got {len(bytes_)} char arrs")
    out = ffi.new("int[]", [-1] * size)

    print("Getting hamming...")
    lib.cluster_ham_and_mark(bytes_, size, out)

    print("Done.")

    with open(f"{path}_out.json", "w") as fw:
        fw.write(json.dumps([i for i in out], indent=4))


if __name__ == "__main__":
    path = sys.argv[1]

    run_libfastcompare(path)
