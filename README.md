# Fast Lazy Hamming Compare

## Compiling

You can run `install.sh` to install compile the library dynamically. Either using GCC or Clang.

The script accepts two flas `-i` and `-c`. `-c` can either be `clang` or `gcc`. `-i` does not have a value but if you pass it, then `clean` directive won't run.

The script will echo your options.

Run it like this:

```
bash ./install.sh -i -c gcc
```

or 

```
bash ./install.sh -c clang
```

Or simply 

```
bash ./install.sh
```

Remember that a shebang has been added to the script. It won't run with `sh`. you will have to run it with `bash`. Either that or do this ONCE:

```
sudo chmod +x ./install.sh
```

Then run it with no issue. Script needs sudo at some point so this will fix it.

## Building FFI

To build the Python FFI using cffi you'll need `cffi`. Create a new virtual env using `python3 -m venv env` and then enter `source env/bin/activate`. You are now in the venv. Install `pip3 install cffi`. Then run:

`python3 ffi.py`

## Running the Script

To run the script you'll need `biopython`. Install it in the same venv. Then run the script and pass the FASTA file like so:

```
python3 fastcompare.py <file>
```

A resulting JSON file will be created. Currently the JSON file does not map to the seqs and headers. It just shows seq `#n` (at index `#n` in the array in the JSON file) is a duplicate of `#k`, `#k` being the value of the array at and index. Sor for example of `test.fa_out.json` looks like this:

```
[-1, 0, -1, -1, -1]
``

That means sequence 2 in the file is a duplicate of sequence 1 in the file. If a sequence is not a duplicate, then the its value at the index in the array will be `-1`. Mappings will SOON be added!