from argparse import ArgumentParser
from itertools import product
from os.path import isfile
from pathlib import Path
from string import Template
from typing import Dict, Generator, List, Tuple

SIZE_WORKERS = 1024
SIZE_THREADS = 512


def generate_macro_names(
            macro_names: List[str], 
            prefices: List[str], 
            max_nums: List[int], 
            lookup_types: List[int],
            num: int,
            num_threads: int,
        ) -> Dict[str, Tuple[str, List[Generator]]]:
    if not (len(macro_names) == len(prefices) == len(lookup_types) == len(max_nums) == num):
        raise ValueError("Number of inputs is not equal.")

    names = {k: {} for k in range(num_threads)}

    for nt in range(num_threads):
        for i in range(num):
            names[nt][macro_names[i]] = (
                max_nums[i],
                lookup_types[i], 
                product([prefices[i]], range(max_nums[i]))
            )

    return names


def assemble_file(
            static_text: str, 
            signature: str, 
            header_path_template: str,
            header_include_template: str,
            src_path_template: str,
            prosumer_types: List[str],
            prosumer_names: List[str],
            prosumer_max: List[str],
            prosumer_init: str,
            hm_names: Dict[str, Generator]
        ) -> Tuple[List[Dict[str, str]], str]:

    tmpl_sig = Template(signature)
    tmpl_hname = Template(header_path_template)
    tmpl_sname = Template(src_path_template)
    tmpl_inc = Template(header_include_template)
    tmpl_init = Template(prosumer_init)

    out = []
    prosumer_array_names = []
    prosumer_array_sigs = []


    for nt, hm in hm_names.items():
        final_text = static_text + "\n\n"
        final_header = ""
        final_lut = ""

        for macro, type_args in hm.items():
            n, type_, args = type_args
            
            look_up_table_init = f"extern {type_} lut_{macro.lower()}[{n}] = {{\n"
            this_text = ""

            for arg in args:
                name = f"{arg[0]}_{arg[1]}_{nt}"
                this_text += f"{macro}({name})\n"
                look_up_table_init += "\t&" + name + ", \n"
                final_header += tmpl_sig.substitute(name=name) + ";\n"

            final_text += this_text + "\n"
            final_lut += look_up_table_init + "};\n\n"

        final_prosumers = ""
        substituted = [tmpl_init.substitute(no_and_nt=f"{i}_{nt}") for i in range(n)]
        prosumer_inits = " ,\n\t".join(substituted)

        for name, ty, mx in zip(
                            prosumer_names,
                            prosumer_types, 
                            prosumer_max
                        ):
            final_prosumers += f"extern {ty} {name}_{nt}[{mx}] = $ls;\n"
            ls = "{\n\t" + prosumer_inits + "};"
            final_prosumers = final_prosumers.replace("$ls", ls)

            prosumer_array_names.append(f"{name}_{nt}")
            prosumer_array_sigs.append(f"extern {ty} {name}_{nt}[{mx}]")

        hname = tmpl_hname.substitute(no=nt)
        sname = tmpl_sname.substitute(no=nt)

        out.append({
            "header_path": hname,
            "src_path": sname,
            "header": final_header,
            "src": final_text + "\n\n" + final_lut + "\n\n" + final_prosumers
        })

    prosumer_array_sigs.append(f"\n\nextern {prosumer_types[0]} *all_drivers[{nt + 1}]")

    headers_all = []

    for n in hm_names.keys():
        headers_all.append(tmpl_inc.substitute(no=n))

    drivers_all = f"{static_text}\n\n\nextern {prosumer_types[0]} *all_drivers[{nt + 1}] = " + "{\n\t" + ',\n\t'.join(prosumer_array_names) + "\n};\n"

    return out, "\n".join(headers_all) + "\n\n\n" + ";\n".join(prosumer_array_sigs), drivers_all


def save_file(ptxts: List[Dict[str, str]], all_headers: str, all_drivers: str, all_headers_path: str, all_drivers_path):
    p_all = Path(all_headers_path)
    p_all.parent.mkdir(exist_ok=True)
    p_all.touch()
    p_all.write_text(all_headers)

    p_all = Path(all_drivers_path)
    p_all.parent.mkdir(exist_ok=True)
    p_all.touch()
    p_all.write_text(all_drivers)

    p_folder_header = Path(ptxts[0]['header_path']).parent
    p_folder_header.mkdir(exist_ok=True)


    p_src_header = Path(ptxts[0]['src_path']).parent
    p_src_header.mkdir(exist_ok=True)

    for d in ptxts:
        pheader = Path(d['header_path'])
        psrc = Path(d['src_path'])

        pheader.touch()
        psrc.touch()

        pheader.write_text(d['header'])
        psrc.write_text(d['src'])


if __name__ == "__main__":
    argp = ArgumentParser()
    argp.add_argument("-nt", "--num_threads", type=int, default=SIZE_THREADS)
    argp.add_argument("-nid", "--num_identifiers", type=int, default=4)
    argp.add_argument("-os", "--src_out", default="generated-src/generated-worker-$no.c")
    argp.add_argument("-oh", "--header_out", default="generated-include/generated-workers/generated-worker-$no.h")
    argp.add_argument("-ha", "--header_all", default="generated-include/generated-all-workers.h")
    argp.add_argument("-da", "--drivers_all", default="generated-src/generated-all-drivers.c")
    argp.add_argument("-st", "--static_txt", default="#include \"../include/fastcompare.h\"")
    argp.add_argument("-hi", "--header_inc_tmp", default="#include \"./generated-workers/generated-worker-$no.h\"")
    argp.add_argument("-pf", "--prefices", nargs="*", default=["PRO", "CON", "DRV", "PRM"])
    argp.add_argument("-mc", "--macros", nargs="*", default=["FUNC_PRO", "FUNC_CON", "FUNC_DRV", "FUNC_PRM"])
    argp.add_argument("-tp", "--types", nargs='*', default=["fptr_t", "fptr_t", "fptr_t", "fptr_t"])
    argp.add_argument("-mx", "--maxn", nargs="*", type=int, default=[SIZE_WORKERS, SIZE_WORKERS, SIZE_WORKERS, SIZE_WORKERS])
    argp.add_argument("-sg", "--signature", default="void $name(void *ptr)")
    argp.add_argument("-pct", "--prosumer_types", nargs="*", default=["driver_s"])
    argp.add_argument("-pcn", "--prosumer_names", nargs="*", default=["drivers"])
    argp.add_argument("-pcx", "--prosumer_max", nargs="*", type=int,  default=[SIZE_WORKERS])
    argp.add_argument("-psi", "--prosumer_init", default="(driver_s){.func_producer=PRO_$no_and_nt, .func_consumer=CON_$no_and_nt, .func_promose=PRM_$no_and_nt, .func_driver=DRV_$no_and_nt, .signal_producer=struct async_sem, .signal_consumer=struct async_sem, .signal_promise=struct async_sem, .driver_pt=struct async, .consumer_pt=struct async, .promise_pt=struct async}")
    argp.add_argument("-stt", "--stat", action="store_true")


    args = argp.parse_args()

    hm = generate_macro_names(
        args.macros, 
        args.prefices, 
        args.maxn, 
        args.types, 
        args.num_identifiers,
        args.num_threads,
    )

    txt_fin, header_fin, drivers_fin = assemble_file(
        args.static_txt,
        args.signature,
        args.header_out,
        args.header_inc_tmp,
        args.src_out,
        args.prosumer_types,
        args.prosumer_names,
        args.prosumer_max,
        args.prosumer_init,
        hm
    )

    save_file(txt_fin, header_fin, drivers_fin, args.header_all, args.drivers_all)

    print("File successfully saved. Pass `-stt` to stat all the files after creation.")

    if args.stat:
        print("--stat passed, getting all the files status...")

        print(f"{args.header_all} OK? {isfile(args.header_all)}")
        print(f"{args.drivers_all} OK? {isfile(args.drivers_all)}")

        for f in txt_fin:
            print(f"{f['header_path']} OK? {isfile(f['header_path'])}")
            print(f"{f['src_path']} OK? {isfile(f['src_path'])}")


        print("Fin!")