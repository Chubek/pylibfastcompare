import sys
from itertools import product
from multiprocessing import Pool
from time import time
from typing import Dict, List, Tuple

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

def fastcompare(cluster: List[Tuple[str, str]], return_dups: bool) -> List[Tuple[str, str]]:
    if len(cluster) < 2:
        return cluster
    
    rows = list(map(lambda x: x[1], cluster))
    rows_delimited = delimit(rows).encode('ascii')
    s_delim = ffi.new("char[]", rows_delimited)
    out = ffi.new("int[]", [0] * len(rows))

    lib.find_hammings_and_mark(s_delim, out)

    filtered = []
    cntr = Counter()
    mark_to_use = int(return_dups)

    def filter_cluster(cntr=cntr, out=out, filtered=filtered):
        if out[cntr()] == mark_to_use:
            filtered.append(cluster[cntr()])

        +cntr

    list(map(lambda _: filter_cluster(), range(len(rows))))

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


def run_concurrently(clusters: Dict[str, List[Tuple[str, str]]], num_proc=6, return_dups=False) -> List[Tuple[str, str]]:
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


def run_pylibfastcompare(path: str, num_proc=6, return_dups=False) -> List[Tuple[str, str]]:
    clusters = read_to_clusters(path)
    fin = run_concurrently(clusters, num_proc, return_dups)

    return fin

if __name__ == "__main__":
    path = sys.argv[1]
    clusters = read_to_clusters(path)
    run_concurrently(clusters=clusters)
