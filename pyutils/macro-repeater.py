from itertools import product
from argparse import ArgumentParser
from typing import Dict, Generator, List, Tuple
from pathlib import Path

def generate_macro_names(
            macro_names: List[str], 
            prefices: List[str], 
            max_nums: List[int], 
            lookup_types: List[int],
            num: int
        ) -> Dict[str, Tuple[str, List[Generator]]]:
    if len(macro_names) != len(prefices) != len(lookup_types) != len(max_nums) != num:
        raise ValueError("Number of inputs is not equal.")

    names = {}

    for i in range(num):
        names[macro_names[i]] = (lookup_types[i], product(prefices[i], int(max_nums[i])))

    return names


def assemble_file(static_text: str, hm_names: Dict[str, Generator]) -> str:
    final_text = static_text + "\n\n"
    final_lut = ""

    for macro, type_args in hm_names:
        type_, args = type_args
        
        look_up_table_init = f"extern {type_} lut_{macro.lower()}[{len(args)}] = {{"
        this_text = ""

        for arg in args:
            name = f"{arg[0]}_{arg[1]}"
            this_text += f"{macro}({name})\n"
            look_up_table_init += "&" + name + ", "

        final_text += this_text + "\n"
        final_lut += look_up_table_init + "}\n\n"

    
    return final_lut + "\n\n" + final_text


def save_file(path: str, txt: str):
    p = Path(path)
    p.parent.mkdir(exist_ok=True)

    p.touch()
    p.write_text(txt)

    print("Generated files successfully written to " + path)


if __name__ == "__main__":
    argp = ArgumentParser()
    argp.add_argument("-n", "--num", default=2)
    argp.add_argument("-o", "--output", default="src/generated.c")
    argp.add_argument("-st", "--statictxt", default="#include \"../include/fastcompare.h\"")
    argp.add_argument("-pf", "--prefices", nargs="*", default="PRO CON")
    argp.add_argument("-mc", "--macros", nargs="*", default="FUNC_PRO FUNC_CON")
    argp.add_argument("-tp", "--types", nargs='*', default="fptr fptr")
    argp.add_argument("-mx", "--maxn", default="1024 1024")

    args = argp.parse_args()

    hm = generate_macro_names(
        args.macros, 
        args.prefices, 
        args.maxn, 
        args.types, 
        args.num
    )

    txt_fin = assemble_file(
        args.statictxt,
        hm
    )

    save_file(args.output, txt_fin)