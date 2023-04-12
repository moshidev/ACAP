run: allgather_exe
	mpirun -n 4 --oversubscribe $< 'ACAP es la mejor asignatura jeje'
	
allgather_exe: allgather.c
	mpicc $< -o $@

clean:
	rm *_exe
