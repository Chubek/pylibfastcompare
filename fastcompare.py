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

class Node:
    def __init__(self, data=None):
        self.data = data
        self.right = None
        self.left = None
        self.eq = None


class TST:
    def __init__(self):
        self.root = Node()
        self.leaf = None

    @staticmethod
    def _search(node, leaf):
        while node:
            if node.data == leaf:
                return node
            if leaf < node.data:
                node = node.left
            else:
                node = node.right
        return None

    def _insert(self, node, leaf):
        if node is None:
            return leaf
        elif leaf.data == node.data:
            return node
        elif leaf.data < node.data:
            node.left = self._insert(node.left, leaf)
        else:
            node.right = self._insert(node.right, leaf)
        return node

    def insert(self, word):
        node = self.root
        for char in word:
            child = self._search(node.eq, char)
            if not child:  # not null
                # create a new node
                child = Node(char)
                node.eq = self._insert(node.eq, child)
            node = child
        if not self._search(node.eq, self.leaf):  # not null
            node.eq = self._insert(node.eq, Node(self.leaf))

    def search(self, word):
        node = self.root
        for c in word:
            node = self._search(node.eq, c)
            if not node:
                return False
        return self._search(node.eq, self.leaf) is not None

    def _traverse(self, node, leaf):
        if node:
            for c in self._traverse(node.left, leaf):
                yield c
            if node.data == leaf:
                yield []
            else:
                for c in self._traverse(node.eq, leaf):
                    yield [node.data] + c
            for c in self._traverse(node.right, leaf):
                yield c

    def traverse(self):
        for w in self._traverse(self.root.eq, self.leaf):
            print(''.join(w))

    def common_prefix(self, chars):
        node = self.root
        buff = []
        for char in chars:
            buff.append(char)
            node = self._search(node.eq, char)
            if not node:
                return
        for x in self._traverse(node.eq, self.leaf):
            yield ''.join(buff + x)

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
    
    for seq_lead, curr_items in prior.items():
        seq_lead = seq_lead.split("_")[0]
        curr_lead = seq_lead

        tst = TST()
        
        for header, seq in curr_items:
            tst.insert(seq + "___" + header)
        already_added = []

        for header, seq in curr_items:
            if (header, seq) in already_added: continue

            curr_lead_len = len(curr_lead)
            new_lead = curr_lead

            for char in seq[curr_lead_len:]:
                new_lead += char

                common_list = list(tst.common_prefix(new_lead))
                if not common_list: break

                already_added.extend(common_list)
            
            already_added = list(set(already_added))
            clusters[new_lead] = [s.split("___") for s in already_added]


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
    clusters = read_to_hashmap(clusters, leads, limit=limit)

    fin = run_concurrently(clusters, num_proc)

    return fin
