NPROCS := 3
MPIRUN_FLAGS := -n $(NPROCS) --oversubscribe
MPICC_FLAGS :=
.PRECIOUS: img/p%w_filter.pgm img/%w_filter.pgm	# tal vez sería mejor crear una variable, realmente para make son archivos intermedios...

test: test_moshi512 test_zzz test_IMG_9986 test_ineitor test_never_ask_how test_nitori_get_down
	@echo todos los tests correctos!

test_%: img/%w_filter.pgm img/p%w_filter.pgm
	diff $^
	@echo '\033[1;32m *' test $*.pgm correcto! '\033[0m'

img/%w_filter.pgm: img/%.pgm $(wildcard ejemplo/*)
	make -C ejemplo -e IIMG=../$< -e OIMG=../$@ ../$@ 

img/p%w_filter.pgm: img/%.pgm pmoshi_filter_exe 
	mpirun $(MPIRUN_FLAGS) ./pmoshi_filter_exe $< $@

pmoshi_filter_exe: ejercicio_2.c ejemplo/pgm.c ejemplo/pgm.h
	mpicc $(MPICC_FLAGS) $(filter %.c, $^) -Iejemplo -o $@

clean:
	rm -f img/*_filter.pgm
	make -C ejemplo clean