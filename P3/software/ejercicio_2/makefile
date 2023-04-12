run: ejercicio_2_exe
	./$< 6 1000000

ejercicio_2_exe: ejercicio_2.c
	gcc -lpthread -lm $^ -o $@

clean:
	rm -f *_exe