#!/usr/bin/env bash

# Input is currentState($1) and totalState($2) (c) Teddy Skarin. https://github.com/fearside/ProgressBar/blob/master/progressbar.sh
function ProgressBar {
	let _progress=(${1}*100/${2}*100)/100
	let _done=(${_progress}*4)/10
	let _left=40-$_done

	_done=$(printf "%${_done}s")
	_left=$(printf "%${_left}s")

	printf "\rProgress : [${_done// /#}${_left// /-}] ${_progress}%%"
}

target=./x64/Debug/P4.exe
nblocks=256
niter_bench=11
img_len=$((100*1024*1024))
fout=experimento_2.csv

RANDOM=0	# seed

APROX_NUM_ITER=256
MAX_NTHREADS_PER_BLOCK=$((1024))
MAX_INCREMENT=$((MAX_NTHREADS_PER_BLOCK / APROX_NUM_ITER))

echo 'nblocks,nthreads_per_block,niter_bench,img_len,gpu_compute_time_seconds' > $fout
nthreads_per_block=1
while [ $nthreads_per_block -le $MAX_NTHREADS_PER_BLOCK ]
do
	ProgressBar $((nthreads_per_block-1) $MAX_NTHREADS_PER_BLOCK
	out=`$target $nblocks $nthreads_per_block $niter_bench $img_len | tail -n 2 | head -n 1`
	gpu_compute_time_seconds=`echo $out | cut -d ':' -f 2 | sed -e 's/,/./g' -e 's/ //g'`
	echo $nblocks,$nthreads_per_block,$niter_bench,$img_len,$gpu_compute_time_seconds >> $fout
	nthreads_per_block=$((nthreads_per_block + 1 + RANDOM % MAX_INCREMENT))
done
ProgressBar 1 1

