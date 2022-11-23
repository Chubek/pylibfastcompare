import os
from pathlib import Path
import pprint
import string
import sys
from multiprocessing import Pool
from typing import Dict, List, Tuple

import cffi
import numpy as np
import tqdm
from random import randint, choice
from Bio.SeqIO.FastaIO import SimpleFastaParser
from phymmr_cluster import get_max_and_pad, read_fasta_get_clusters

from _fastcompare import ffi, lib
from mmapfastaparser import MmapFastaParser

if 'FASTCOM_SO' in os.environ:
    if os.environ['FASTCOMP_SO'] == "1":
        ffi = cffi.FFI()

DICT_DUP_LABEL = {
    1: "DUP",
    0: "NONDUP"
}

class Counter:
    def __init__(self) -> None:
        self.cnt = 0
    
    def __pos__(self):
        self.cnt += 1

    def __call__(self) -> int:
        return self.cnt

def fastcompare(cluster: List[Tuple[str, str]]) -> Dict[List[Dict[str, str]], Dict[str, Dict[str, str]]]:
    if len(cluster) < 2:
        return {"Clean": {cluster[0][0]: cluster[0][1]}, "Dupes": {}}

    if len(cluster) % 2 != 0:
        cluster = cluster[:len(cluster) - 1]

    seqs_sep = list(map(lambda x: x[1], cluster))
    rows = len(seqs_sep)
    maxlen = len(seqs_sep[0]) + 1

    input_list = []
    def to_byte_and_pad(ins: str, input_list=input_list):
        inst_bytes_list = list(ins.encode('ascii'))
        inst_bytes_padded = inst_bytes_list + [0]
        input_list.append(inst_bytes_padded)
    list(map(to_byte_and_pad, seqs_sep))

    arr_int = ffi.cast(f"char*[{rows}]", np.asanyarray(input_list, dtype=np.uint8).ctypes.data)
    out = ffi.new("int[]", [-1] * rows)

    lib.find_hammings_and_mark(
        arr_int,
        out,
        rows,
        maxlen
    )

    kicked_and_masters = {}
    deduped = {}
    cntr = Counter()

    def filter_cluster(
        cntr=cntr, 
        out=out, 
        kicked_and_masters=kicked_and_masters,
        deduped=deduped
    ):
        master_ind = out[cntr()]
        subject_header, subject_seq = cluster[cntr()]
        if master_ind > -1:
            master_header, _ = cluster[master_ind]
            kicked_and_masters[subject_header] = {"Seq": subject_seq, "Master": master_header}
        else:
            deduped[subject_header] = subject_seq

        +cntr
    list(map(lambda _: filter_cluster(), range(rows)))
    return {"Clean": deduped, "Dupes": kicked_and_masters}


def read_to_clusters(path: str, limit=10, is_large=False) -> Dict[str, List[Tuple[str, str]]]:
    clusters = {}
    recs = MmapFastaParser(Path(path))

    nxt = +recs
    while nxt:
        header, seq = nxt
        key = seq[:limit] + "_" + str(len(seq))
        if is_large:
            key += max(set(seq), key=seq.count) + "_" + min(set(seq), key=seq.count) + "".join(list(set(seq)))

        clusters.setdefault(key, [])
        clusters[key].append((header, seq))
        
        nxt = +recs
 
    return clusters


def run_concurrently(
    clusters: Dict[str, List[Tuple[str, str]]], 
    num_proc=6
) -> Dict[str, Dict[str, str]]:
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

    all_in_one_dict = {"Dupes": {}, "Clean": {}}
    for d in fin:
        if d is None: continue

        dedup = d['Clean']
        kicked = d['Dupes']

        if len(dedup) > 0:
            for k1, v1 in dedup.items():
                all_in_one_dict["Clean"][k1] = v1

        if len(kicked) > 0:
            for k2, v2 in kicked.items():
                all_in_one_dict["Dupes"][k2] = v2

    print(f"Got {len(all_in_one_dict['Dupes'])} dupes")
    return all_in_one_dict
    


def run_pylibfastcompare(
    path: str, num_proc=6, limit=10, subsequent_max_size=20
) -> Dict[str, Dict[str, str]]:
    is_large = (os.stat(path).st_size // 1000000) > subsequent_max_size
    clusters = read_to_clusters(
        path, 
        limit=limit, 
        is_large=is_large
    )

    fin = run_concurrently(clusters, num_proc)
    
    return fin

if __name__ == "__main__":
    path = sys.argv[1]
    clusters = read_to_clusters(path)
    run_concurrently(clusters=clusters)
