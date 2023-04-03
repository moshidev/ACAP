NPROCS := 3
MPIRUN_FLAGS := -n $(NPROCS) --oversubscribe
MPICC_FLAGS :=

test: img/moshi512w_filter.pgm img/pmoshi512w_filter.pgm
	diff $^

img/moshi512w_filter.pgm: img/moshi512.pgm $(wildcard ejemplo/*)
	make -C ejemplo -e IIMG=../$< -e OIMG=../$@ ../$@ 

img/pmoshi512w_filter.pgm: img/moshi512.pgm pmoshi_filter_exe 
	mpirun $(MPIRUN_FLAGS) ./pmoshi_filter_exe $< $@

pmoshi_filter_exe: ejercicio_2.c ejemplo/pgm.c ejemplo/pgm.h
	mpicc $(MPICC_FLAGS) $(filter %.c, $^) -Iejemplo -o $@

clean:
	rm -f img/*_filter.pgm
	make -C ejemplo clean