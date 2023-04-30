#!/bin/bash

function wait_until_no_jobs() {
	njobs=$((`squeue | grep acap19 | wc -l`))
	while [[ $njobs -eq 1 ]] 
	do 
		sleep 1
		njobs=$((`squeue | grep acap19 | wc -l`))
	done 
}

run='sbatch -Aacap -p acap --exclusive --hint=nomultithread -N 1 -c 1 -n 6 --wrap'
#run='bash -c'
output_file=ppi.csv
nprocs=6
nhyperthreads=2

make

wait_until_no_jobs
$run "mpirun -np 1 ./ppi_exe 1 1 | head -n 1 > $output_file"
for i in `seq 1 1 $nprocs`
do
	for t in `seq 1 1 $(($nprocs*$nhyperthreads / $i))`
	do
		wait_until_no_jobs
		$run "mpirun -np $i ./ppi_exe 2147483647 $t | tail -n 1 >> $output_file"
	done
done

wait_until_no_jobs
