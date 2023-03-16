/**
 * Daniel Pedrosa Montes © 2023
 * Ejercicio 2 de la primera práctica de ACAP.
 */

// Calculo aproximado de PI mediante la serie de Leibniz e integral del cuarto de circulo
// https://es.wikipedia.org/wiki/Serie_de_Leibniz
// N.C. Cruz, Universidad de Granada, 2023

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <sys/time.h>

double get_wall_time() {
    struct timeval time;
    assert(gettimeofday(&time,NULL) == 0);
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

double get_cpu_time() {
    return (double)clock() / CLOCKS_PER_SEC;
}

double piLeibniz(int steps){
	double partpi = 0.0;
	double num = 1.0;
	double denom = 1.0;
	for(int i = 0; i<steps; i++){
		partpi += num/denom;
		num = -1.0*num; // Alternamos el signo
		denom += 2.0;
	}
	return 4.0*partpi;
}

double piRectangles(int intervals){
	double width = 1.0/intervals;
	double sum = 0.0, x;
	for(int i = 0; i<intervals; i++){
		x = (i + 0.5)*width;
		sum += 4.0/(1.0 + x*x);
	}
	return sum*width;
}

int main(int argc, char* argv[]){
	if(argc!=2){	//El primer argumento siempre es el nombre del programa
		printf("Uso: ./prog esfuerzo\n");
	}else{
		int steps = atoi(argv[1]);
		if(steps<=0){
			printf("El número de iteraciones debe ser un entero positivo!\n");
		}else{
            printf("num_iter,leibniz_cput,leibniz_wallt,rectangles_cput,rectangles_wallt,leibniz_pi,rectangles_pi\n");
            for (int i = 0; i < steps; i++) {
                int iter = (i+1) * ((double)INT_MAX / steps);
                double cput1 = get_cpu_time();
                double wallt1 = get_wall_time();
				double pi1 = 0.0;
                for (int i = 0; i < 5; i++) {
                    pi1 += piLeibniz(iter);
                }
                cput1 = (get_cpu_time() - cput1) / 5;
                wallt1 = (get_wall_time() - wallt1) / 5;
				pi1 /= 5;
    
                double cput2 = get_cpu_time();
                double wallt2 = get_wall_time();
				double pi2 = 0.0;
                for (int i = 0; i < 5; i++) {
                    pi2 += piRectangles(iter);
                }
                cput2 = (get_cpu_time() - cput2) / 5;
                wallt2 = (get_wall_time() - wallt2) / 5;
				pi2 /= 5;

                printf("%d,%lf,%lf,%lf,%lf,%lf,%lf\n", iter, cput1, wallt1, cput2, wallt2, pi1, pi2);
            }
        }
	}
	return 0;
}