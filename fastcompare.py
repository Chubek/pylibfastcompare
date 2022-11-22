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
        return None

    if len(cluster) % 2 != 0:
        cluster = cluster[:len(cluster) - 1]

    seqs_sep = list(map(lambda x: x[1], cluster))
    maxlen = len(max(seqs_sep, key=lambda x: len(x))) + 1
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
        if bool(master_ind):
            master_header, _ = cluster[master_ind]
            kicked_and_masters[subject_header] = {"Seq": subject_seq, "Master": master_header}
        else:
            deduped[subject_header] = subject_seq

        +cntr
    list(map(lambda _: filter_cluster(), range(rows)))
    return {"Dedup": deduped, "Kicked": kicked_and_masters}


def read_to_clusters(path: str, limit=10) -> Dict[str, List[Tuple[str, str]]]:
    clusters = {}
    recs = MmapFastaParser(Path(path))

    nxt = +recs
    while nxt:
        header, seq = nxt
        clusters.setdefault(seq[:limit], [])
        clusters[seq[:limit]].append((header, seq))
        
        nxt = +recs
 
    return clusters


def subsequent_clusters(clusters: Dict[str, List[Tuple[str, str]]]) -> Dict[str, List[Tuple[str, str]]]:
    clusters_fin = {}
    
    for k, v in clusters.items():
        len_cluster = len(v)        
        
        if len_cluster > 300:
            denum_rand = 2 if len_cluster > 2000 else 4
            rands = [randint(0, len_cluster) for _ in range(len_cluster // denum_rand)]
            
            for header, seq in v:
                l = len(seq)
                most_common = max(seq, key=seq.count)
                least_common = min(seq, key=seq.count)
                rand_id = choice(rands)

                key = f"{k}-{l}-{most_common}-{least_common}-{rand_id}"
                clusters_fin.setdefault(key, [])
                clusters_fin[key].append((header, seq))

        else:
            clusters_fin[k] = v

    return clusters_fin

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


    all_in_one_dict = {}
    for d in fin:
        if d is None: continue

        all_in_one_dict.setdefault("Dedup", {})
        all_in_one_dict.setdefault("Kicked", {})
        dedup = d['Dedup']
        kicked = d['Kicked']

        try:
            for k, v in dedup.items():
                all_in_one_dict['Dedup'][k] = v
        except:
            pass

        try:
            for k, v in kicked.items():
                all_in_one_dict['Kicked'][k] = v
        except:
            pass

    return all_in_one_dict
    


def run_pylibfastcompare(
    path: str, num_proc=6, limit=10
) -> Dict[str, Dict[str, str]]:
    clusters = read_to_clusters(path, limit=limit)
    clusters = subsequent_clusters(clusters=clusters)
    fin = run_concurrently(clusters, num_proc)
    
    return fin

if __name__ == "__main__":
    path = sys.argv[1]
    clusters = read_to_clusters(path)
    run_concurrently(clusters=clusters)
