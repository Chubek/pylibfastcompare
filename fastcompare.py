import os
from multiprocessing import Pool
from pathlib import Path
from typing import Dict, List, Tuple

import cffi
import numpy as np
import tqdm

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

    seqs_sep = list(map(lambda x: x[1], cluster))
    rows = len(seqs_sep)
    maxlen = len(seqs_sep[0]) + 1

    input_list = []

    def to_byte_and_pad(ins: str, input_list=input_list):
        inst_bytes_list = list(ins.encode('ascii'))
        inst_bytes_padded = inst_bytes_list + [0]
        input_list.append(inst_bytes_padded)
    list(map(to_byte_and_pad, seqs_sep))

    arr_int = ffi.cast(
        f"char*[{rows}]", np.asanyarray(input_list, dtype=np.uint8).ctypes.data)
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
    out = [i for i in out]

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
            kicked_and_masters[subject_header] = {
                "Seq": subject_seq, "Master": master_header}
        else:
            deduped[subject_header] = subject_seq

        +cntr
    list(map(lambda _: filter_cluster(), range(rows)))
    return {"Clean": deduped, "Dupes": kicked_and_masters}


def read_to_prior_hashmap(
    path: str,
    limit=10
) -> Tuple[Dict[str, Tuple[str, str]], Dict[str, Tuple[int, int]]]:
    prior = {}
    leads = {}
    recs = MmapFastaParser(Path(path))

    nxt = +recs
    while nxt:
        header, seq = nxt
        key = seq[:limit] + "_" + str(len(seq))
        prior.setdefault(key, [])
        prior[key].append((header, seq))
        leads.setdefault(key, [0, 0])
        leads[key][0] += 1
        leads[key][1] += len(seq)

        nxt = +recs

    return prior, leads


def read_to_hashmap(
    prior: Dict[str, Tuple[str, str]],
    leads: Dict[str, Tuple[int, int]],
    limit=10
):
    clusters = {}

    for h, (num, sum_lens) in leads.items():
        if num > 1000:
            dividend = 4.5

            if num > 5000:
                dividend = 4.25
            if num > 10000:
                dividend = 4.00
            if num > 30000:
                dividend = 3.75
            if num > 60000:
                dividend = 3.25
            if num > 90000:
                dividend = 3.00
            if num > 120000:
                dividend = 2.75

            avg_lens = sum_lens // num
            seq_placeholder = "." * avg_lens
            limit_new = limit

            while len(seq_placeholder[:limit_new]) < int(avg_lens // dividend):
                limit_new += 1

            limit = limit_new

        all_items = prior[h]
        for head, seq in all_items:
            len_seq = len(seq)
            key = seq[:limit] + "_" + str(len_seq)

            clusters.setdefault(key, [])
            clusters[key].append((head, seq[limit:]))

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
        all_in_one_dict['Dupes'].update(d['Dupes'])
        all_in_one_dict['Clean'].update(d['Clean'])

    print(f"Got {len(all_in_one_dict['Dupes'])} dupes")
    return all_in_one_dict


def run_pylibfastcompare(
    path: str, num_proc=6, limit=10, subsequent_max_size=20
) -> Dict[str, Dict[str, str]]:
    clusters, leads = read_to_prior_hashmap(path, limit)
    clusters = read_to_hashmap(clusters, leads)

    fin = run_concurrently(clusters, num_proc)

    return fin
