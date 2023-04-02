$(OIMG): $(IIMG) moshi_filter_exe
	./moshi_filter_exe $< $@

moshi_filter_exe: sinbordes.c pgm.c pgm.h
	gcc $(filter %.c, $^) -o $@

clean:
	rm -f *_exe