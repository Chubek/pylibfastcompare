from fastcompare import run_pylibfastcompare

FILE = "EOG7B0GZN.aa.fa"
NUM_PROC = 6
RETURN_DUPS = False

res = run_pylibfastcompare(FILE, NUM_PROC, RETURN_DUPS)

print(res)