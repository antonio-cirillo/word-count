#!/bin/bash

log_file="./test/test.txt"

file1="./test/word-count.csv"
file2="./word-count.csv"

rm $log_file

make clean 2>/dev/null
make benchmark 2>/dev/null

for i in {2..100} 
do

    echo "Number of processes: $i" >> $log_file
    echo "----------------------------------------------------------------" >> $log_file
    mpirun --mca btl_vader_single_copy_mechanism none \
        -np $i --allow-run-as-root \
        ./word-count -d /home/test_files >> $log_file

    if cmp -s "$file1" "$file2"; then
        echo "Test passed!" >> $log_file
    else
        echo "Test not passed!" >> $log_file
    fi

    echo "" >> $log_file

    rm word-count.csv

done

make clean 2>/dev/null 