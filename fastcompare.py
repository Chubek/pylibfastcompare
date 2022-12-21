import json
import sys
from os import environ
from time import time
from typing import Any, Dict, List, Tuple
import cffi
from Bio.SeqIO.FastaIO import SimpleFastaParser
import argparse
import gc

from _fastcompare import lib

ffi = cffi.FFI()

K = 4
ZERO_ENCODED = '\0'.encode('ascii')
    
def seqs_bytes(
    path: str,
    thread_div: int
) -> Tuple[bytearray, List[Tuple[str, str]], int]:
    rec_reader = SimpleFastaParser(open(path, "r"))
    buffers = []
    hseqs = []
    n = 0

    for header, seq in rec_reader:
        buffers.append(ffi.new("uint8_t[]", (seq.encode('ascii') + ZERO_ENCODED)))
        hseqs.append((header, seq))
        n += 1

    return buffers, hseqs, n

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
    buffer, haader_seqs, size = seqs_bytes(path, thread_div)
    print(f"Got {size} char arrs. Took {time() - t} seconds")
    out = ffi.new("int[]", [-1] * size)
    t = time()
    print("Getting hamming with FFI...")
    lib.cluster_ham_and_mark(buffer, size, K, out)
    out = [i for i in out]
    print(f"Done deduping. Took {time() - t} seconds. Exporting results...")
    print(f"Found {len([i for i in out if i != -1])} dups")
    
    ret = {"Clean": {}, "Dupes": {}}

    for i in range(size):
        h, s = haader_seqs[i]        
        res = out[i]

        if res == -1:
            ret["Clean"][h] = s
        else:
            h_master, _ = haader_seqs[res]
            ret["Dupes"][h] = {"Seq": s, "Master": h_master}

    with open(f"{path}_out.json", "w") as fw:
        fw.write(json.dumps(ret, indent=4, sort_keys=True))   

    print(f"Results exprted to {path.split('.')[0]}_out.json")

def trace(frame, event, arg):
    print("%s, %s:%d" % (event, frame.f_code.co_filename, frame.f_lineno))
    return trace

if __name__ == "__main__":
    argp = argparse.ArgumentParser()
    argp.add_argument("-i", "--input", type=str, required=True)
    argp.add_argument("-p", "--parallel", default=5, type=int, required=False)
    argp.add_argument("-st", "--strace", action="store_true", required=False)

    args = argp.parse_args()
    
    if args.strace:
        sys.settrace(trace)

    run_libfastcompare(args.input, args.parallel)
