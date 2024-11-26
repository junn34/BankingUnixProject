CC = gcc
CFLAGS = -Wall -g
TARGETS = server client

all: $(TARGETS)

server: server.c include/banking.h
	$(CC) $(CFLAGS) -o server server.c

client: client.c include/banking.h
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f $(TARGETS)
	rm -rf data/*.dat

.PHONY: all clean

