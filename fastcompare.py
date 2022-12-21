import json
import sys
from concurrent.futures import ThreadPoolExecutor
from os import environ
from threading import Thread
from time import time
from typing import Any, Dict, List, Tuple
import numpy as np

import cffi
from Bio.SeqIO.FastaIO import SimpleFastaParser

from _fastcompare import lib

ffi = cffi.FFI()

K = 4
    
def seqs_bytes(
    path: str,
    thread_div: int
) -> Tuple[bytearray, List[int], List[Tuple[str, str]], int]:
    rec_reader = SimpleFastaParser(open(path, "r"))
    lengths = []
    header_seqs = list(rec_reader)
    seqs = list(map(lambda x: x[1], header_seqs))
    seq = "".join(seqs).encode('ascii')
    n = len(seqs)

    
    with ThreadPoolExecutor(thread_div) as pool:
        for l in pool.map(len, seqs):
            lengths.append(l)
    
    buffer = np.frombuffer(seq, dtype=np.uint8)
    buffer = ffi.from_buffer(buffer)

    return buffer, lengths, header_seqs, n

def assembler_worker(size: int, start: int, end: int, heads_seqs: List[Tuple[str, str]], arr_in: List[int], dict_out: Dict[str, Dict[str, str]]): 
    for i in range(start, end):
        if i >= size: break

        h, s = heads_seqs[i]
        res = arr_in[i]

        if res == -1:
            dict_out["Clean"][h] = s
        else:
            h_master, _ = heads_seqs[res]
            dict_out["Dupes"][h] = {"Seq": s, "Master": h_master}


def run_libfastcompare(path: str, thread_div: int):
    print('Reading, getting bytes...')
    t = time()
    buffer, lengths, hseqs, size = seqs_bytes(path, thread_div)
    print(f"Got {size} char arrs. Took {time() - t} seconds")
    out = ffi.new("int[]", [-1] * size)
    t = time()
    print("Getting hamming with FFI...")
    lib.cluster_ham_and_mark(buffer, lengths, size, K, out)
    out = [i for i in out]
    print(out)
    print(f"Done deduping. Took {time() - t} seconds. Exporting results...")
    print(f"Found {len([i for i in out if i != -1])} dups")
    
    thrds = []
    ret = {"Clean": {}, "Dupes": {}}

    for i in range(0, size, size // thread_div):
        t = Thread(target=assembler_worker, args=(size, i, i + (size // thread_div), hseqs, [i for i in out], ret, ))
        thrds.append(t)
        t.start()

    [t.join() for t in thrds]

    with open(f"{path}_out.json", "w") as fw:
        fw.write(json.dumps(ret, indent=4, sort_keys=True))

    print(f"Results exprted to {path}_out.json")


if __name__ == "__main__":
    path = sys.argv[1]
    thread_div = int(sys.argv[2]) if len(sys.argv) > 2 else 32

    run_libfastcompare(path, thread_div)
