#!/bin/bash

log_file="./benchmark/strong-scalability.txt"

base="test_files/list1.txt test_files/list2.txt test_files/list3.txt"
input=""

make clean 2>/dev/null
make benchmark 2>/dev/null

rm $log_file
touch $log_file

for i in {2..24} 
do

    input="$input $base"
    echo $input

    echo "Number of processes: $i" >> $log_file
    echo "----------------------------------------------------------------" >> $log_file
    mpirun --mca btl_vader_single_copy_mechanism none \
        -np $i --hostfile benchmark/hfile \
        ./word-count -f $input >> $log_file

    echo "" >> $log_file

    rm word-count.csv

done

make clean 2>/dev/null