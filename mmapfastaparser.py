from mmap import PROT_READ, mmap
from pathlib import Path
from typing import Tuple

class MmapFastaParser:
    def __init__(self, filepath: Path):
        with open(filepath, "r+b") as fbr:
            mmpf = mmap(fbr.fileno(), 0, prot=PROT_READ)

        self.mmp_iter = iter(mmpf.readline, b"")
        self.curr_line = next(self.mmp_iter)


    def __next__(self) -> Tuple[str, str]:
        try:
            header = self.curr_line.decode().strip()[1:]
            seq = next(self.mmp_iter).decode().strip()
            
            while True:
                self.curr_line = next(self.mmp_iter)

                if self.curr_line[0] == ord('>'):
                    return (header, seq)
                else:
                    seq += self.curr_line.decode().strip()

        except:
            return None


    def __pos__(self) -> Tuple[str, str]:
        return self.__next__()


aa = MmapFastaParser("EOG7B0GZN.aa.fa")
print(next(aa))