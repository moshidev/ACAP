#ifndef __PGM__
#define __PGM__

void ** GetMem2D(int rows, int columns, int sizeofTipo);
void Free2D(void ** h, int rows);

unsigned char** pgmread(char* filename, int* x_size, int* y_size);
int pgmwrite(unsigned char** Imagen, char* filename, int Largo, int Alto);

#endif /*__PGM__*/
