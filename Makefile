CC = gcc
CFLAGS = -Wall -Wextra -pedantic -ggdb
LDFLAGS = -lraylib -lm

SOURCES = main.c

main: 
	$(CC) $(CFLAGS) $(SOURCES) -o main $(LDFLAGS)
