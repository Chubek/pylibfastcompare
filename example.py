from fastcompare import run_pylibfastcompare

FILE = "SRR12778644.fa"
NUM_PROC = 6
RETURN_DUPS = False

res = run_pylibfastcompare(FILE, NUM_PROC, RETURN_DUPS)

print(res)