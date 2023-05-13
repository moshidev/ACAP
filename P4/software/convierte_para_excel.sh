#!/usr/bin/env bash

for i in `seq 1 1 3`
do
	echo Convierte experimento_"$i".csv en excel_experimento_"$i".csv.
	cat experimento_"$i".csv | sed -e 's/,/:/g' -e 's/\./,/g' > excel_experimento_"$i".csv
done
