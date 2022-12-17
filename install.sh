#!/bin/bash

cc="gcc"
ggd=""
while getopts i?d?c: flag
do
    case "${flag}" in
        i) init="init";;
        c) cc=${OPTARG};;
        d) ggd="-ggdb3";; 
    esac
done

echo "Compiler is $cc"

if [ "$init" = "init" ]
then
    echo "init enabled, rm will not run"
fi

if [ "$ggd" = "-ggdb3" ]
then
    echo "Running with debug flag, are you sure?"
fi

make $init COMP=$cc DEBUG=$ggd
sudo make install
python3 ffi.py