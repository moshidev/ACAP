run: ejercicio_1_exe
	./$< 6 1000000

ejercicio_1_exe: ejercicio_1.c
	gcc -lpthread -lm $^ -o $@

clean:
	rm -f *_exe