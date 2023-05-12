#!/usr/bin/env bash

# Input is currentState($1) and totalState($2) (c) Teddy Skarin. https://github.com/fearside/ProgressBar/blob/master/progressbar.sh
function ProgressBar {
	let _progress=(${1}*100/${2}*100)/100
	let _done=(${_progress}*4)/10
	let _left=40-$_done

	_done=$(printf "%${_done}s")
	_left=$(printf "%${_left}s")

	printf "\rProgress : [${_done// /#}${_left// /-}] ${_progress}%%\r"
}

target=./x64/Debug/P4.exe
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
	ProgressBar $((nblocks-1)) $MAX_BLOCKS
	out=`$target $nblocks $nthreads_per_block $niter_bench $img_len | tail -n 2 | head -n 1`
	gpu_compute_time_seconds=`echo $out | cut -d ':' -f 2 | sed -e 's/,/./g' -e 's/ //g'`
	echo $nblocks,$nthreads_per_block,$niter_bench,$img_len,$gpu_compute_time_seconds >> $fout
	nblocks=$((nblocks + 1 + RANDOM % MAX_INCREMENT))
done
ProgressBar 1 1

echo ¡Resultados escritos correctamente al fichero $fout!
