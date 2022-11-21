from functools import reduce
import sys
from itertools import product
from multiprocessing import Pool
from time import time
from typing import Dict, List, Tuple
import numpy as np

import cffi
from Bio.SeqIO.FastaIO import SimpleFastaParser
from phymmr_cluster import get_max_and_pad, read_fasta_get_clusters

from _fastcompare import ffi, lib

DELIM = '|'

#ffi = cffi.FFI()

class Counter:
    def __init__(self) -> None:
        self.cnt = 0
    
    def __pos__(self):
        self.cnt += 1

    def __call__(self) -> int:
        return self.cnt

def fastcompare(cluster: List[Tuple[str, str]], return_dups: bool) -> Dict[int, List[Tuple[str, str]]]:
    if len(cluster) < 2:
        return cluster
    
    rows = list(map(lambda x: x[1], cluster))
    maxlen = len(max(rows, key=lambda x: len(x)))
    len_row = len(rows)

    mapped_np = []
    def map_to_nparray(s: str, mapped_np=mapped_np):
        mapped_np.append(list(s.encode('ascii')) + ([0] * (maxlen - len(s))))

    list(map(map_to_nparray, rows))
    mapped_np = np.asanyarray(mapped_np, dtype=np.uint8)
    char_arr = ffi.cast(f"const unsigned char*[{len_row}]", mapped_np.ctypes.data)
    out = ffi.new("int[]", [0] * len_row)

    lib.find_hammings_and_mark(char_arr, out, len_row, maxlen)

    filtered = {}
    cntr = Counter()
    mark_to_use = int(return_dups)

    def filter_cluster(cntr=cntr, out=out, filtered=filtered):
        filtered.setdefault(out[cntr()], [])
        filtered[out[cntr()]].append(cluster[cntr()])

        +cntr

    list(map(lambda _: filter_cluster(), range(len_row)))

    return filtered


def delimit(lstr: List[str]) -> str:
    return DELIM.join(lstr) + DELIM

def read_to_clusters(path: str, limit=10) -> Dict[str, List[Tuple[str, str]]]:
    recs = SimpleFastaParser(open(path, 'r'))
    clusters = {}

    for header, row in recs:
        clusters.setdefault(row[:limit], [])
        clusters[row[:limit]].append((header, row))

    return clusters


def run_concurrently(clusters: Dict[str, List[Tuple[str, str]]], num_proc=6, return_dups=False) -> List[Dict[int, List[Tuple[str, str]]]]:
    print("Starting pool...")

    fin = []

    with Pool(num_proc) as pool:
        for res in pool.starmap(
           fastcompare,
            product(clusters.values(), [return_dups]),
            chunksize=1000
        ):
            fin.extend(res)   

    return fin


def run_pylibfastcompare(path: str, num_proc=6, return_dups=False) -> List[Dict[int, List[Tuple[str, str]]]]:
    clusters = read_to_clusters(path)
    fin = run_concurrently(clusters, num_proc, return_dups)

    return fin

if __name__ == "__main__":
    path = sys.argv[1]
    clusters = read_to_clusters(path)
    run_concurrently(clusters=clusters)
