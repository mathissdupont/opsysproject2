CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic -std=c11
LDLIBS ?= -lpthread

all: it_support ai_researchers

it_support: it_support.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

ai_researchers: ai_researchers.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -f it_support ai_researchers *.o

.PHONY: all clean
