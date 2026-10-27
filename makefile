CC     = gcc
CFLAGS = -Wall -O2 -I. -include scheduler.h

ALGORITHMS = rr constant exponential custom

all: $(ALGORITHMS)

$(ALGORITHMS): %: algorithms/%.c simulator.c scheduler.h
	$(CC) $(CFLAGS) -o $@ algorithms/$*.c simulator.c

clean:
	rm -f $(ALGORITHMS)

.PHONY: all clean
