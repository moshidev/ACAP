#!/usr/bin/env bash

# Este algoritmo, a diferencia del resto, requiere que se compile y ejecute el algoritmo implementado en la CPU y que se muestre por pantalla su tiempo

# Input is currentState($1) and totalState($2) (c) Teddy Skarin. https://github.com/fearside/ProgressBar/blob/master/progressbar.sh
function ProgressBar {
	let _progress=(${1}*100/${2}*100)/100
	let _done=(${_progress}*4)/10
	let _left=40-$_done

	_done=$(printf "%${_done}s")
	_left=$(printf "%${_left}s")

	printf "\rProgress : [${_done// /#}${_left// /-}] ${_progress}%%\r"
}

# Shows a spinner while another command is running. Randomly picks one of 12 spinner styles.
# @args command to run (with any parameters) while showing a spinner. 
#       E.g. ‹spinner sleep 10› https://unix.stackexchange.com/a/565551
function shutdown() {
  tput cnorm # reset cursor
}
trap shutdown EXIT

function cursorBack() {
  echo -en "\033[$1D"
}

function spinner() {
  # make sure we use non-unicode character type locale 
  # (that way it works for any locale as long as the font supports the characters)
  local LC_CTYPE=C

  local pid=$1 # Process Id of the previous running command
  local spin='⣾⣽⣻⢿⡿⣟⣯⣷'
  local charwidth=3
  local i=0
  tput civis # cursor invisible
  while kill -0 $pid 2>/dev/null; do
    local i=$(((i + $charwidth) % ${#spin}))
    printf "%s" "${spin:$i:$charwidth}"

    cursorBack 1
    sleep .1
  done
  tput cnorm
  wait $pid # capture exit code
  return $?
}

target=./x64/Debug/P4.exe
nblocks=3060
nthreads_per_block=512
niter_bench=11
img_len=$((100*1024*1024))
fout=experimento_3.csv

fout_tmp=."$RANDOM"_experimento_3.out
$target $nblocks $nthreads_per_block $niter_bench $img_len | sed 's/,/./g' > $fout_tmp &
spinner $!

tiempo_medio_gpu_s=`cat $fout_tmp | tail -n 2 | head -n 1 | cut -d ':' -f 2 | sed 's/ //g'`
echo ¡Obtenido tiempo medio de ejecución en GPU!
rm -f $fout_tmp

tiempo_cpu_s=0.0
tiempo_con_transferencia_s=0.0
for i in `seq 1 1 $niter_bench`
do
	ProgressBar $((i-1)) $niter_bench
	out=`$target $nblocks $nthreads_per_block 1 $img_len | sed 's/,/./g'`
	cpu_s=`echo "$out" | head -n 1 | cut -d ':' -f 2 | sed 's/ //g'`
	gpu_transfer_s=`echo "$out" | tail -n 1 | cut -d ':' -f 2 | sed 's/ //g'`
	tiempo_cpu_s=`echo "$cpu_s + $tiempo_cpu_s" | bc -l`
	tiempo_con_transferencia_s=`echo "$gpu_transfer_s + $tiempo_con_transferencia_s" | bc -l`
done
ProgressBar 1 1
tiempo_cpu_s="0"`echo "scale=8; $tiempo_cpu_s / $niter_bench" | bc -l`
tiempo_con_transferencia_s="0"`echo "scale=8; $tiempo_cpu_s / $niter_bench" | bc -l`

echo tiempo_cpu_s,tiempo_medio_gpu_s,tiempo_con_transferencia_s > $fout
echo $tiempo_cpu_s,$tiempo_medio_gpu_s,$tiempo_con_transferencia_s >> $fout

echo ¡Resultados escritos correctamente al fichero $fout!
