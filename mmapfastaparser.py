from mmap import PROT_READ, mmap
from pathlib import Path
from typing import Tuple

class MmapFastaParser:
    def __init__(self, filepath: Path):
        with open(filepath, "r+b") as fbr:
            mmpf = mmap(fbr.fileno(), 0, prot=PROT_READ)

        self.mmp_iter = iter(mmpf.readline, b"")


    def __next__(self) -> Tuple[str, str]:
        try:
            header = next(self.mmp_iter).decode().strip()[1:]
            seq = next(self.mmp_iter).decode().strip()

            return (header, seq)
        except:
            return None


    def __pos__(self) -> Tuple[str, str]:
        return self.__next__()
