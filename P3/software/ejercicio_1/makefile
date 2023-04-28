run: ejercicio_1_exe
	./$< 6 1000000000

ejercicio_1_exe: ejercicio_1.c
	gcc $^ -o $@ -lpthread -lm

clean:
	rm -f *_exe