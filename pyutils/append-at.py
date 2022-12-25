from pathlib import Path
import re
from argparse import ArgumentParser
import sys

def append_file(
            path: str, 
            appendage: str, 
            pattern: str, 
            from_top=False,
            upsert=False,
        ):
    p = Path(path)
    txt = p.read_text()

    split = txt.split("\n")

    if append_file in split and not upsert:
        print('File already contains appendage and upsert is false. Aborting...')
        sys.exit(0)
    
    f = lambda l: reversed(list(range(l))) if not from_top else list(range(l))

    for i in f(len(split)):
        line = split[i]
        if re.search(pattern, line) is None: continue

        print("Pattern found.")
        split = split[:i + 1] + [appendage] + split[i + 1:]

        break

    p.write_text("\n".join(split))
    print(f"Appended `{appendage}` to {path}. Attempt may have failed. But if it says `Pattern found`, most probqably not.")


if __name__ == "__main__":
    argp = ArgumentParser()
    argp.add_argument("-i", "--input", default="include/fastcompare.h")
    argp.add_argument("-p", "--pattern", default="#include")
    argp.add_argument("-a", "--appendage", default="#include \"../generated-include/generated-workers.h\"")
    argp.add_argument("-t", "--topwise", action="store_true")
    argp.add_argument("-u", "--upsert", action="store_true")

    args = argp.parse_args()

    append_file(args.input, args.appendage, args.pattern, args.topwise, args.upsert)
