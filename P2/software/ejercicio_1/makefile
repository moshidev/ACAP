ejercicio_1_exe: ejercicio_1.c
	mpicc $< -o $@ -lm

run: ejercicio_1_exe
	mpirun -n 4 --oversubscribe $<
