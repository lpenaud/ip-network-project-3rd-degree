CC=gcc
CFLAGS=-W -Wall -pedantic -std=gnu99
LDFLAGS=

all: index.html concert.out places.out achat.out chat_server.out

index.html: README.md design/*
	pandoc -f gfm -t html5 -o $@ -s $< --metadata-file=design/metadata.yaml --toc

concert.out: objects/concert.o objects/helpers.o
	$(CC) $^ -o $@ $(LDFLAGS)

places.out: objects/places.o objects/helpers.o
	$(CC) $^ -o $@ $(LDFLAGS)

achat.out: objects/achat.o objects/helpers.o
	$(CC) $^ -o $@ $(LDFLAGS)

chat_server.out: objects/chat_server.o
	$(CC) $^ -o $@ $(LDFLAGS)

objects/helpers.o: src/helpers.h

objects/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	-rm index.html
	-rm *.out
	-rm objects/*.o
