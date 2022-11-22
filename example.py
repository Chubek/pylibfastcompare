from fastcompare import run_pylibfastcompare
import json

FILE = "EOG73NSQM.aa.fa"
NUM_PROC = 12
RETURN_DUPS = False

res = run_pylibfastcompare(FILE, NUM_PROC, RETURN_DUPS)
with open("ss.json", "w") as fw:
    fw.write(json.dumps(res, indent=4, sort_keys=True))
