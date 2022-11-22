from fastcompare import run_pylibfastcompare
import json

FILE = "SRR12778644.fa"
NUM_PROC = 48
CLUSTER_LEAD_NUM_CHR = 25

res = run_pylibfastcompare(FILE, NUM_PROC, CLUSTER_LEAD_NUM_CHR)
with open("res.json", "w") as fw:
    fw.write(json.dumps(res, indent=4, sort_keys=True))

