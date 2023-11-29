CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: server client word-count

server: server.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

word-count: main.c wordsProcessorLL.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f server client word-count
