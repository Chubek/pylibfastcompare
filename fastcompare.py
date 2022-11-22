import pprint
import sys
from multiprocessing import Pool
from typing import Dict, List, Tuple

import cffi
import numpy as np
import tqdm
from Bio.SeqIO.FastaIO import SimpleFastaParser
from phymmr_cluster import get_max_and_pad, read_fasta_get_clusters
import os

from _fastcompare import ffi, lib

if 'FASTCOM_SO' in os.environ:
    if os.environ['FASTCOMP_SO'] == "1":
        ffi = cffi.FFI()

DICT_DUP_LABEL = {
    1: "DUP",
    0: "NONDUP "
}

class Counter:
    def __init__(self) -> None:
        self.cnt = 0
    
    def __pos__(self):
        self.cnt += 1

    def __call__(self) -> int:
        return self.cnt

def fastcompare(cluster: List[Tuple[str, str]]) -> Dict[int, List[Tuple[str, str]]]:
    if len(cluster) < 2:
        return {-1: cluster}

    if len(cluster) % 2 != 0:
        cluster = cluster[:len(cluster) - 1]

    seqs_sep = list(map(lambda x: x[1], cluster))
    maxlen = len(max(seqs_sep, key=lambda x: len(x)))
    rows = len(seqs_sep)

    input_list = []
    def to_byte_and_pad(ins: str, input_list=input_list):
        inst_bytes_list = list(ins.encode('ascii'))
        inst_bytes_padded = inst_bytes_list + ([0] * (maxlen - len(ins)))
        input_list.append(inst_bytes_padded)
    list(map(to_byte_and_pad, seqs_sep))

    arr_int = ffi.cast(f"char*[{rows}]", np.asanyarray(input_list, dtype=np.uint8).ctypes.data)
    out = ffi.new("int[]", [0] * rows)

    lib.find_hammings_and_mark(
        arr_int,
        out,
        rows,
        maxlen
    )

    filtered = {}
    cntr = Counter()

    def filter_cluster(cntr=cntr, out=out, filtered=filtered):
        filtered.setdefault(DICT_DUP_LABEL[out[cntr()]], [])
        filtered[DICT_DUP_LABEL[out[cntr()]]].append(cluster[cntr()])

        +cntr

    list(map(lambda _: filter_cluster(), range(rows)))
    return filtered


def read_to_clusters(path: str, limit=10) -> Dict[str, List[Tuple[str, str]]]:
    clusters = {}
    for header, seq in SimpleFastaParser(open(path, 'r')):
        clusters.setdefault(seq[:limit], [])
        clusters[seq[:limit]].append((header, seq))

    return clusters


def run_concurrently(
    clusters: Dict[str, List[Tuple[str, str]]], 
    num_proc=6, return_dups=False
) -> List[Dict[int, List[Tuple[str, str]]]]:
    print("Starting pool...")
    clusters_sorted = sorted(list(clusters.values()), key=lambda x: len(x))
    clusters_sorted.reverse()



    with Pool(num_proc) as pool:
        fin = list(
            tqdm.tqdm(
                pool.imap(
                   fastcompare,
                   clusters_sorted,
                   chunksize=1000
               ), 
                total=len(clusters_sorted)
            )
       )


    return fin


def run_pylibfastcompare(path: str, num_proc=6, return_dups=False) -> List[Dict[int, List[Tuple[str, str]]]]:
    clusters = read_to_clusters(path)
    fin = run_concurrently(clusters, num_proc, return_dups)
    return fin

if __name__ == "__main__":
    path = sys.argv[1]
    clusters = read_to_clusters(path)
    run_concurrently(clusters=clusters)
