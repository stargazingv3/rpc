CC=gcc
CFLAGS=-g -Wall
LDFLAGS=-lpthread

all: server client

server: server.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f server client
