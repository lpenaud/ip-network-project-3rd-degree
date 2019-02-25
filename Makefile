CC=gcc
CFLAGS=-W -Wall -pedantic
LDFLAGS=

all: index.html concert.out places.out achat.out

index.html: README.md design/*
	pandoc -f gfm -t html5 -o $@ -s $< --metadata-file=design/metadata.yaml --toc

concert.out: objects/concert.o objects/helpers.o
	$(CC) $^ -o $@ $(LDFLAGS)

places.out: objects/places.o objects/helpers.o
	$(CC) $^ -o $@ $(LDFLAGS)

achat.out: objects/achat.o objects/helpers.o
	$(CC) $^ -o $@ $(LDFLAGS)

objects/helpers.o: objects/helpers.h

objects/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	-rm index.html
	-rm *.out
	-rm objects/*.o
