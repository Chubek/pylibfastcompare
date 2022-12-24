input=""
debug=""
while getopts i:c?s?e?d?v? flag
do
    case "${flag}" in
        i) input=${OPTARG};;
        c) input="c.fa";;
        s) input="68.fa";;
        e) input="83.fa";;
        v) debug="valgrind";;
        d) debug="gdb -ex r --args"        
    esac
done

echo "Running pylibfastcompare with input " $input

if [ "$debug" = "gdb -ex r --args" ]
then
    echo "Debug mode enabled"
fi


$debug python3 fastcompare.py -i $input