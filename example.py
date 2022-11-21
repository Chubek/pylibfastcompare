from fastcompare import run_pylibfastcompare

FILE = "EOG73NSQM.aa.fa"
NUM_PROC = 12
RETURN_DUPS = False

res = run_pylibfastcompare(FILE, NUM_PROC, RETURN_DUPS)

print(res)