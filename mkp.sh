#!/usr/bin/env bash

nombre_practica=$1

if [ $# -lt 1 ] || [ $# -gt 1 ]
then
    echo "No puedo ejecutar el script si me das un número de argumentos que no esperaba. Uso: $0 P[0-9]." 1>&2
    exit 1
fi

if ! echo "$nombre_practica" | grep 'P[0-9]' 1> /dev/null
then
    echo "No puedo ejecutar el script si no utilizas un nombre de práctica válido. Uso: $0 P[0-9]." 1>&2
    exit 2
fi

if [ -e $nombre_practica ]
then
    echo "No puedo ejecutar el script si ya existe un directorio con el mismo nombre que \"$1\"." 1>&2
    exit 3
fi

mkdir $nombre_practica $nombre_practica/software $nombre_practica/memoria

git clone 'https://github.com/moshidev/plantilla_memoria_TeX' $nombre_practica/memoria
rm -rf $nombre_practica/memoria/.git
mkdir $nombre_practica/memoria/input
