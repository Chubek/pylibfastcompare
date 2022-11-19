import sys
from multiprocessing import Pool

from time import time
from typing import Dict, List, Tuple

import cffi
import numpy as np
from Bio.SeqIO.FastaIO import SimpleFastaParser
from phymmr_cluster import get_max_and_pad, read_fasta_get_clusters

from _fastcompare import lib

BIT_ZERO = ''.encode('ascii')

ffi = cffi.FFI()


def fastcompare(cluster: List[Tuple[bytearray, Tuple[str, str]]]) -> List[Tuple[str, str]]:
    input_list, rows, cols = get_max_and_pad([cl[0] for cl in cluster])
    intacts = [cl[1] for cl in cluster]

    arr_int = ffi.from_buffer(np.asanyarray(input_list, dtype=np.uint8))
    out = ffi.new("int[]", [0] * rows)

    lib.iterate_and_eliminate(
        arr_int,
        rows,
        cols,
        out
    )

    return [intacts[i] for i in range(rows) if out[i] == 1]


def read_to_clusters(path: str, limit=10) -> List[List[Tuple[bytearray, Tuple[str, str]]]]:
    recs = list(SimpleFastaParser(open(path, 'r')))

    return read_fasta_get_clusters(recs, limit)


def run_concurrently(clusters: List[List[Tuple[bytearray, Tuple[str, str]]]]):
    print("Starting pool...")

    fin = []

    with Pool(6) as pool:
        for res in pool.map(
            fastcompare,
            clusters,
            chunksize=1000
        ):
            fin.append(res)


if __name__ == "__main__":
    path = sys.argv[1]
    clusters = read_to_clusters(path)
    run_concurrently(clusters=clusters)
