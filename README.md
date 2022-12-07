# Fast Lazy Hamming Compare

## Compiling

An install script has been provided for you. 
Run it like this:

```
sh ./install.sh <compiler> <init?>
```

`<compler>` can be either `gcc` or `clang`. This will compile in either using the `-mavx2` flag which both share. Your CPU must support AVX2.

For example `sh ./install.sh gcc`

`<init>` is an optional argument. If passed, it won't run `clean` directive. Some distros have issues with removing files that don't exist. So if `libfastcompare.so` does not exist in your directory, run it like this:

```
sh ./install.sh gcc init
```

The scripts prompts for password because `make install` requires sudo privileges. To avoid having to enter password over and over again do this ONCE:

```
sudo chmod +x ./install.sh
```

And then you can run it without `sh` and it will not prompt anymore.

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