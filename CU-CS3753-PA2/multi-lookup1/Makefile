CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -Wall -Wextra -pthread
multi-lookup: multi-lookup.o
	$(CC) $(LFLAGS) -o multi-lookup multi-lookup.o

multi-lookup.o: multi-lookup.c multi-lookup.h
	$(CC) $(CFLAGS) multi-lookup.c

clean:
	rm -f multi-lookup
	rm -f *.o
	rm -f *~