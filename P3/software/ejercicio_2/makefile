run: ejercicio_2_exe
	./$< 6 1000000000

ejercicio_2_exe: ejercicio_2.c
	gcc $^ -o $@ -lpthread -lm

clean:
	rm -f *_exe