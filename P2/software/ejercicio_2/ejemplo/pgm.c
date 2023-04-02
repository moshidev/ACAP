#include "pgm.h"

#include <stdlib.h>
#include <stdio.h>

void * GetMem(size_t Num, size_t Tam) {
	void * p;

	p = calloc(Num, Tam);
	if (p == NULL) {
		fprintf(stderr, "Error in asigned memory.\n");
		fflush(stderr);
		exit(1);
	}
	return p;
}

void ** GetMem2D(int rows, int columns, int sizeofTipo) {//Contigua
	int i;
	void ** h;

  	void * h_container = calloc(rows*columns, sizeofTipo);

 	h = malloc(sizeof(void*)*rows);
  	for(int i = 0; i<rows; i++){
    		h[i] = h_container + (i*sizeofTipo*columns);
  	}

  	return h;
}

void Free2D(void ** h, int rows) {//Contigua
  free(h[0]);
  free(h);
}

int getint(FILE *fd){
  int c, i;
  char dummy[10000];

  c = getc(fd);
  while (1) /* buscar el siguiente entero */
    {
      if (c=='#')    /* Descartar los comentarios */
        fgets(dummy,9000,fd);
      if (c==EOF){
        return 0;
        }
      if (c>='0' && c<='9')
        break;   /* se ha encontrado lo que se buscaba */
      c = getc(fd);
    }

  /* se esta al comienzo de un numero, avanzar hasta el final */
  i = 0;
  while (1) {
    i = (i*10) + (c - '0');
    c = getc(fd);
    if (c==EOF) return (i);
    if (c<'0' || c>'9') break;
  }

  return (i);
}

unsigned char** pgmread(char* filename, int* x_size, int* y_size) {
  FILE  *fd;
  char  header[100];
  int   tmp;
  int   x, y, Contador=0;
  unsigned char **Imagen;

  if ((fd=fopen(filename,"rb")) == NULL){
    printf("No puedo abrir %s para lectura!\n", filename);
    exit(0);
  }

  /* Leyendo el encabezado */
  header[0]=fgetc(fd);
  header[1]=fgetc(fd);
  if(!(header[0]=='P' && header[1]=='5')){
    printf("Error al leer el archivo\nLa imagen %s no es un PGM!\n",filename);
    fclose(fd);
    exit(0);
  }

  *x_size = getint(fd);
  *y_size = getint(fd);
  tmp = getint(fd);

  Imagen = (unsigned char **)GetMem2D(*x_size, *y_size, sizeof(unsigned char));

  /* Toma la imagen del archivo */
  for (y=0; y<*y_size; y++)
    for (x=0; x<*x_size; x++, Contador++)
      Imagen[x][y]=(unsigned char)fgetc(fd);

  /* Verifico que se leyeron todos los bytes */
  if (Contador != (*x_size * *y_size)){
    printf("Archivo con longitud incorrecta!\n");
    exit(1);
  }

  fclose(fd);
  printf("Cargando imagen '%s' de %ix%i\n", filename, *x_size, *y_size);
  return Imagen;
}

int pgmwrite(unsigned char** Imagen, char* filename, int Largo, int Alto) {
  FILE *Out;
  int x, y, Contador=0;

  if ((Out=fopen(filename,"wb")) == NULL){
    printf("No puedo escribir la imagen!\n");
    return 0;
  }

  /* Ponemos el encabezado de los PGM */
  fprintf(Out,"P5\n");
  fprintf(Out,"%d %d\n", Largo, Alto);
  fprintf(Out,"255\n");

  /* Vacio la imagen al archivo */
  for (y=0; y<Alto; y++)
    for (x=0; x<Largo; x++, Contador++)
      fputc(Imagen[x][y], Out);

  /* Verifico que se escriban todos los bytes */
  if (Contador != (Largo*Alto)){
    printf("Archivo con longitud incorrecta!\n");
    return 0;
  }

  fclose(Out);

  printf("\nGuardando imagen '%s' de %ix%i\n", filename, Largo, Alto);
  return 1;
}
