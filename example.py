from fastcompare import run_pylibfastcompare
import json

FILE = "EOG73NSQM.aa.fa"
NUM_PROC = 48
CLUSTER_LEAD_NUM_CHR = 10
SUBSEQUENT_MAX_SIZE = 20

res = run_pylibfastcompare(FILE, NUM_PROC, CLUSTER_LEAD_NUM_CHR, SUBSEQUENT_MAX_SIZE)
with open(f"{FILE}.json", "w") as fw:
    fw.write(json.dumps(res, indent=4, sort_keys=True))

