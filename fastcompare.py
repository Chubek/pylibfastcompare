import os
import time
from multiprocessing import Pool
from pathlib import Path
from typing import Dict, List, Tuple, Union

import cffi
import numpy as np
import tqdm

from _fastcompare import lib
from mmapfastaparser import MmapFastaParser

SIZE_CHARS = 32
SIZE_OUT = 4
SIZE_HAM = 4

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


def cluster_on_len(
    path: str,
) -> Tuple[Dict[str, Tuple[str, str]], Dict[str, Tuple[int, int]]]:
    clusters_length = {}

    recs = MmapFastaParser(Path(path))

    nxt = +recs
    while nxt:
        header, seq = nxt
        key = f"{len(seq)}|{seq[:10]}"
        clusters_length.setdefault(key, [])
        clusters_length[key].append((header, seq))

        nxt = +recs

    return clusters_length


def pack_cluster(cluster: List[Tuple[str, str]]) -> List[Tuple[List[int], Tuple[str, str]]]:
    seqs_secluded = list(map(lambda x: x[1], cluster))
    seqs_secluded_list_of_encoded_chars = list(map(lambda x: list(
        x.encode('ascii')) + ([0] * (len(x) % SIZE_CHARS)), seqs_secluded))

    ret = []

    for j, seq_ascii in enumerate(seqs_secluded_list_of_encoded_chars):
        inner_list = []
        for i in range(0, len(seq_ascii), SIZE_CHARS):
            arr_in = ffi.cast(f"uint8_t[{SIZE_CHARS}]", np.asanyarray(
                seq_ascii[i:i + SIZE_CHARS], dtype=np.uint8).ctypes.data)
            out = ffi.new("uint64_t[1]", [0])

            lib.pack_32_bytes_in_64_bits(arr_in, out)
            inner_list.append(out[0])

        ret.append((inner_list, cluster[j]))

    return ret


def get_hamming_pair(a: List[int], b: List[int]) -> bool:
    a = a + ([0] * (len(a) % SIZE_HAM))
    b = b + ([0] * (len(b) % SIZE_HAM))

    result = 0

    for i in range(0, len(a), SIZE_HAM):
        sub_a = a[i:i + SIZE_HAM]
        sub_b = b[i:i + SIZE_HAM]

        sub_a_arr = ffi.cast(
            f"uint64_t[{SIZE_HAM}]", np.asanyarray(sub_a, dtype=np.uint64).ctypes.data)
        sub_b_arr = ffi.cast(
            f"uint64_t[{SIZE_HAM}]", np.asanyarray(sub_b, dtype=np.uint64).ctypes.data)
        out = ffi.new("int[1]", [0])

        lib.get_hamming_integers(sub_a_arr, sub_b_arr, out)

        result += out[0]

    True if result < 2 else False


def get_hamming_cluster(
    cluster: List[Tuple[List[int], Tuple[str, str]]]
) -> Dict[
    str, Union[Dict[str, str],
               Dict[str, Dict[str, str]]
               ]
]:
    packed_integers = list(map(lambda x: x[0], cluster))
    items = list(map(lambda x: x[1], cluster))

    results = [-1] * len(packed_integers)

    for i, a in enumerate(packed_integers):
        if results[i] != -1:
            continue

        for j, b in enumerate(packed_integers[i + 1:]):
            if results[j] != -1:
                continue

            if get_hamming_pair(a, b):
                results[j] = i

    ret = {"Clean": {}, "Dupes": {}}

    cntr = Counter()

    def filter_cluster(
        cntr=cntr,
        results=results,
        ret=ret,
        items=items,
    ):
        master_ind = results[cntr()]
        subject_header, subject_seq = items[cntr()]
        if master_ind > -1:
            master_header, _ = cluster[master_ind]
            ret["Dupes"][subject_header] = {
                "Seq": subject_seq, "Master": master_header}
        else:
            ret["Clean"][subject_header] = subject_seq

    list(map(lambda _: filter_cluster(), range(len(items))))

    return ret


def pack_clusters(clusters: Dict[int, List[Tuple[str, str]]], p=6) -> List[List[Tuple[List[int], Tuple[str, str]]]]:
    print("Packing clusters --- started pool...")

    with Pool(p) as pool:
        ret = list(
            tqdm.tqdm(
                pool.imap(
                    pack_cluster,
                    clusters.values(),
                    chunksize=1000
                ),
                total=len(clusters)
            )
        )

    return ret


def run_concurrently(
    clusters: Dict[int, List[Tuple[str, str]]],
    num_proc=6
) -> Dict[str, Dict[str, str]]:
    clusteds_packed = pack_clusters(clusters, p=num_proc)

    print("Doing hamming compare, started pool...")

    with Pool(num_proc) as pool:
        fin = list(
            tqdm.tqdm(
                pool.imap(
                    get_hamming_cluster,
                    clusteds_packed,
                    chunksize=1000
                ),
                total=len(clusteds_packed)
            )
        )

    all_in_one_dict = {"Dupes": {}, "Clean": {}}

    for d in fin:
        all_in_one_dict['Dupes'].update(d['Dupes'])
        all_in_one_dict['Clean'].update(d['Clean'])

    print(f"Got {len(all_in_one_dict['Dupes'])} dupes")
    return all_in_one_dict


def run_pylibfastcompare(
    path: str, num_proc=6
) -> Dict[str, Dict[str, str]]:
    print("Begenning clustering on length...")
    t = time.time()

    clusters = cluster_on_len(path)
    fin = run_concurrently(clusters, num_proc)

    print(f"Finished. Took {time.time() - t} seconds.")

    return fin
