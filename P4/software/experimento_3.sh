#!/usr/bin/env bash

target=./mock.sh
nblocks=256
nthreads_per_block=256
niter_bench=11
img_len=$((100*1024*1024))
fout=experimento_3.csv

out=`$target $nblocks $nthreads_per_block $niter_bench $img_len | sed 's/,/./g'`
tiempo_medio_gpu_s=`echo "$out" | tail -n 2 | head -n 1 | cut -d ':' -f 2 | sed 's/ //g'`

out=`$target $nblocks $nthreads_per_block 1 $img_len | sed 's/,/./g'`
tiempo_cpu_s=0.0
tiempo_con_transferencia_s=0.0
for i in `seq 1 1 $niter_bench`
do
	cpu_s=`echo "$out" | head -n 1 | cut -d ':' -f 2 | sed 's/ //g'`
	gpu_transfer_s=`echo "$out" | tail -n 1 | cut -d ':' -f 2 | sed 's/ //g'`
	tiempo_cpu_s=`echo "$cpu_s + $tiempo_cpu_s" | bc`
	tiempo_con_transferencia_s=`echo "$gpu_transfer_s + $tiempo_con_transferencia_s" | bc`
done
tiempo_cpu_s="0"`echo "scale=4; $tiempo_cpu_s / $niter_bench" | bc -l`
tiempo_con_transferencia_s="0"`echo "scale=4; $tiempo_cpu_s / $niter_bench" | bc -l`

echo tiempo_cpu_s,tiempo_medio_gpu_s,tiempo_con_transferencia_s > $fout
echo $tiempo_cpu_s,$tiempo_medio_gpu_s,$tiempo_con_transferencia_s >> $fout
