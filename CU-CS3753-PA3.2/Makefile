CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -g -Wall -Wextra

INPUTFILESIZEMEGABYTES = 1

KILO = 1024
MEGA = $(shell echo $(KILO)\*$(KILO) | bc)
INPUTFILESIZEBYTES = $(shell echo $(MEGA)\*$(INPUTFILESIZEMEGABYTES) | bc)
INPUTBLOCKSIZEBYTES = $(KILO)
INPUTBLOCKS = $(shell echo $(INPUTFILESIZEBYTES)\/$(INPUTBLOCKSIZEBYTES) | bc)

.PHONY: all clean

all: cpu-process io-process mixed-process

cpu-process: cpu-process.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

io-process: io-process.o rwinput
	$(CC) $(LFLAGS) io-process.o -o $@ -lm


mixed-process: mixed-process.o rwinput
	$(CC) $(LFLAGS) mixed-process.o -o $@ -lm

cpu-process.o: cpu-process.c
	$(CC) $(CFLAGS) $<

io-process.o: io-process.c
	$(CC) $(CFLAGS) $<

mixed-process.o: mixed-process.c
	$(CC) $(CFLAGS) $<

rwinput: Makefile
	dd if=/dev/urandom of=./rwinput bs=$(INPUTBLOCKSIZEBYTES) count=$(INPUTBLOCKS)

clean: testclean
	rm -f cpu-process io-process mixed-process
	rm -f rwinput
	rm -f *.o
	rm -f *~

testclean:
	rm -f rwoutput*
