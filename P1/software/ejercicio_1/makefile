ejercicio_1_exe: ejercicio_1.c
	mpicc -o $@ $<

run: ejercicio_1_exe
	mpiexec -n 3 $< 5
