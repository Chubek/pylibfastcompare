from fastcompare import run_pylibfastcompare
import json

FILE = "INSbttTARAAPEI-83.fa"
NUM_PROC = 12

res = run_pylibfastcompare(FILE, NUM_PROC)
with open(f"{FILE}.json", "w") as fw:
    fw.write(json.dumps(res, indent=4, sort_keys=True))

