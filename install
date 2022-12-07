#!/bin/bash

cc="gcc"

while getopts i:c: flag
do
    case "${flag}" in
        i) init="init";;
        c) cc=${OPTARG};;
    esac
done

echo "Compiler is $cc"

if [ "$init" = "init" ]
then
    echo "init enabled, rm will not run"
fi

make $init COMP=$cc
sudo make install
python3 ffi.py