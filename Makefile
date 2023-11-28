SHELL := bash
CC := avr-gcc
CFLAGS += -std=c2x -Wall -Wextra -Wpedantic -Werror -Wno-array-bounds -Wno-gnu-binary-literal -Wno-gnu-zero-variadic-macro-arguments -O2 -DF_CPU=16000000UL
MCU := atmega32u4
PARTNO := m32u4
PROGRAMMER := avrispmkII

.PHONY: all program build binsize walk_binsize compile clean

all: program

program: a.out
	avrdude -p $(PARTNO) -c $(PROGRAMMER) -U flash:w:$<:e

build: a.out

binsize: a.bin
	@stat --format="%s" $<

walk_binsize:
	@[ -e clean ] && ./clean || true;
	@[ -e Makefile ] && $(MAKE) -s clean || true;
	@for commit in `git rev-list master ^8de95ac`; do \
		git checkout -q $$commit; \
		git log --oneline -1; \
		[ -e build ] && ./build > /dev/null 2> /dev/null; \
		[ -e binsize ] && ./binsize; \
		[ -e Makefile ] && $(MAKE) -s binsize; \
	done
	@git checkout -q master
	@git log --oneline -1
	@[ -e clean ] && ./clean || true;
	@[ -e Makefile ] && $(MAKE) -s clean || true;

compile: main.o

clean:
	rm -f -- *.out *.bin *.o

main.o: main.c main.h inline_utility.h inline_twi.h
	$(CC) $(CFLAGS) -mmcu=$(MCU) -c $<

a.out: main.o
	$(CC) $(CFLAGS) -mmcu=$(MCU) $<

a.bin: a.out
	@avr-objcopy -O binary $< a.bin
