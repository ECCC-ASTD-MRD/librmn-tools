
# all: emit scan parse scan2
all: scan2

fruit.o: fruit.c fruit.h
	$(CC) -c -g -O2 -Wall fruit.c

emit.o: emit.c fruit.h
	$(CC) -c -g -O2 -Wall emit.c

emit: fruit.o emit.o
	$(CC) -o emit fruit.o emit.o -lyaml

scan.o: scan.c
	$(CC) -c -g -O2 -Wall scan.c

scan2.o: scan2.c
	$(CC) -c -g -O2 -Wall scan2.c

scan: scan.o
	$(CC) -o scan scan.o -lyaml

scan2: scan2.o
	$(CC) -o scan2 scan2.o -lyaml

parse.o: parse.c fruit.h
	$(CC) -c -g -O2 -Wall parse.c

parse: fruit.o parse.o
	$(CC) -o parse fruit.o parse.o -lyaml

clean:
	rm -f emit scan parse
	rm -f *.o core
