make:	memsim
	gcc -o memsim memsim.o
memsim:	memsim.c
	gcc -c memsim.c

clean: 
	rm *.o memsim *.out *.exe