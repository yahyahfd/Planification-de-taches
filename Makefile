make:
	gcc -Wall -o cassini -I include src/cassini.c src/timing-text-io.c -g
distclean:
	rm cassini
