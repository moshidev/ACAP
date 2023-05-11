#!/usr/bin/env bash

target=./mock.sh
nthreads_per_block=256
niter_bench=11
img_len=$((100*1024*1024))
fout=experimento_1.csv

RANDOM=0	# seed

APROX_NUM_ITER=256
MAX_BLOCKS=$((2**16))
MAX_INCREMENT=$((MAX_BLOCKS / APROX_NUM_ITER))

echo 'nblocks,nthreads_per_block,niter_bench,img_len,gpu_compute_time_seconds' > $fout
nblocks=1
while [ $nblocks -le $MAX_BLOCKS ]
do
	out=`$target $nblocks $nthreads_per_block $niter_bench $img_len | tail -n 2 | head -n 1`
	gpu_compute_time_seconds=`echo $out | cut -d ':' -f 2 | sed -e 's/,/./g' -e 's/ //g'`
	echo $nblocks,$nthreads_per_block,$niter_bench,$img_len,$gpu_compute_time_seconds >> $fout
	nblocks=$((nblocks + 1 + RANDOM % MAX_INCREMENT))
done
