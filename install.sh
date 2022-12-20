#!/bin/bash

cc="gcc"
ggd=""
asan=""
init=""
ll="/usr/lib/x86_64-linux-gnu/libasan.so.6"
while getopts i?d?c:a?l: flag
do
    case "${flag}" in
        i) init="init";;
        c) cc=${OPTARG};;
        l) ll=${OPTARG};;
        d) ggd="1";; 
        a) asan="1";;
    esac
done

echo "Compiler is $cc"

if [ "$init" = "init" ]
then
    echo "init enabled, rm will not run"
fi

if [ "$ggd" = "1" ]
then
    echo "Running with debug flag, are you sure?"
fi

if [ "$asan" = "1" ]
then
    echo "Running with ASan flag, library is "$ll
fi

make $init COMP=$cc DEBUG=$ggd ASAN=$asan ASAN_LD=$ll
sudo make install
python3 ffi.py