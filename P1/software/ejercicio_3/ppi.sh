#!/bin/bash

function wait_until_no_jobs() {
	njobs=$((`squeue | wc -l` - 1))
	while [[ $njobs -eq 1 ]] 
	do 
		sleep 1
		njobs=$((`squeue | wc -l` - 1))
	done 
}

run='sbatch -Aacap -p acap --exclusive --hint=nomultithread -N 1 -c 1 -n 6 --wrap'
output_file=ppi.csv

mpicc ppi.c -o ppi_exe

$run "mpirun -np 1 ./ppi_exe 0 > $output_file"
for i in `seq 1 1 6`
do
	wait_until_no_jobs
	$run "mpirun -np $i ./ppi_exe 2147483647 >> $output_file"
done

wait_until_no_jobs
