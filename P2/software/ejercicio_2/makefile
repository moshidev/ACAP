test: img/moshi512w_filter.pgm
	@echo "hola :3"

img/moshi512w_filter.pgm: img/moshi512.pgm $(wildcard ejemplo/*)
	make -C ejemplo ../$@ -e IIMG=../$< -e OIMG=../$@

clean:
	rm -f img/*_filter.pgm
	make -C ejemplo clean