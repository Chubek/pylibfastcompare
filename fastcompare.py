import json
import sys
from threading import Thread
from typing import Any, Dict, List, Tuple
from concurrent.futures import ThreadPoolExecutor

import cffi
from Bio.SeqIO.FastaIO import SimpleFastaParser

from _fastcompare import lib


ffi = cffi.FFI()

def seqs_bytes(
    path: str,
) -> Tuple[List[Tuple[Tuple[str, str], bytearray]], int]:
    list_bytes = []
    rec_reader = SimpleFastaParser(open(path, "r"))

    for header, seq in rec_reader: 
        list_bytes.append(((seq, header), ffi.new("char[]", (seq.encode('ascii') + '\0'.encode('ascii')))))

    return list_bytes, len(list_bytes)

def assembler_worker(size: int, start: int, end: int, heads_seqs: List[Tuple[str, str]], arr_in: List[int], dict_out: Dict[str, Dict[str, str]]): 
    for i in range(start, end):
        if i >= size: break

        h, s = heads_seqs[i]
        res = arr_in[i]

        if res == -1:
            dict_out["Clean"][h] = {"Headr": h, "Seq": s}
        else:
            h_master, _ = heads_seqs[res]
            dict_out["Dupes"][h] = {"Header": h, "Seq": s, "Master": h_master}


def run_libfastcompare(path: str):
    print('Reading, getting bytes...')
    bytes_, size = seqs_bytes(path)
    print(f"Got {size} char arrs")
    out = ffi.new("int[]", [-1] * size)

    print("Getting hamming...")
    lib.cluster_ham_and_mark(list(map(lambda x: x[-1], bytes_)), size, out)

    print("Done deduping, exporting results...")

    hseqs = list(map(lambda x: x[0], bytes_))
    thrds = []
    ret = {"Clean": {}, "Dupes": {}}

    for i in range(0, size, size // 8):
        t = Thread(target=assembler_worker, args=(size, i, i + 2024, hseqs, [i for i in out], ret, ))
        thrds.append(t)
        t.start()

    [t.join() for t in thrds]

    with open(f"{path}_out.json", "w") as fw:
        fw.write(json.dumps(ret, indent=4, sort_keys=True))


if __name__ == "__main__":
    path = sys.argv[1]

    run_libfastcompare(path)
