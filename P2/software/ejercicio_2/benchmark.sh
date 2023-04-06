#!/usr/bin/env bash

rango_inferior=1
rango_superior=6
csv_file=pfilter.csv

if ! [ $# -eq 1 ] || ! echo $1 | grep 'img/.*.pgm' 1> /dev/null
then
    echo "Uso incorrecto. Uso: \`$0 img/[fichero .PGM]\`" >&2
    exit 1
fi

if ! file $1 | grep 'Netpbm' 1> /dev/null
then
    echo "O bien el fichero $1 no existe o bien no es un fichero .PGM" >&2
    exit 2
fi

echo " * Compila el algoritmo paralelo..."
make pmoshi_filter_exe
echo " * ¡Hecho!"

echo " * Ejecuta algoritmo paralelizado..."
rm $csv_file
echo "nprocs,wall_time,cpu_time" | tee -a $csv_file
for i in `seq $rango_inferior 1 $rango_superior`
do
    fout=.pbench_$RANDOM
    job=`mpirun -n $i ./pmoshi_filter_exe $1 $fout`
    wall_time=`echo $job | cut -d ',' -f1 | cut -d ':' -f2`
    cpu_time=`echo $job | cut -d ',' -f2 | cut -d ':' -f2`
    echo $i,$wall_time,$cpu_time | tee -a $csv_file
    rm $fout
done
echo " * ¡Hecho!"
