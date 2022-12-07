import sys
from random import choice, choices, randint
from string import ascii_letters

letters = ['A', 'T', 'G', 'C']

gen_head = lambda i: f">{''.join(choices(ascii_letters, k=10))}|{i + 9000}|{''.join(choices(ascii_letters, k=5))}"
gen_lead = lambda: "".join(choices(letters, k=randint(32, randint(64, 164))))
gen_seqs = lambda leads: f"{choice(leads)}{''.join(choices(letters, k=randint(64, 256)))}"
make_dup = lambda seq: seq.replace(seq[randint(0, len(seq) - 1)], choice(letters)) if randint(0, 10) % 2 == 0 else seq

if __name__ == "__main__":
    fname = sys.argv[1]
    max_num = int(sys.argv[2]) if len(sys.argv) > 2 else 20000

    heads = [gen_head(i) for i in range(max_num)]
    leads = [gen_lead() for i in range(max_num // 5)]
    seqs_main = [gen_seqs(leads) for _ in range(max_num)]
    seqs_dups = [make_dup(seqs_main[i]) for i in range(max_num // 3)]
    seqs = [choice(seqs_dups + seqs_main) for _ in range(max_num)]


    with open(fname, "w") as fw:
        for h, s in zip(heads, seqs):
            fw.write(h + '\n')
            fw.write(s + '\n')
