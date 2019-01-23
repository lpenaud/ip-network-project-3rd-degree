CC=gcc
CFLAGS=-W -Wall -pedantic
LDFLAGS=

all: index.html

index.html: README.md design/*
	pandoc -f gfm -t html5 -o $@ -s $< --metadata-file=design/metadata.yaml --toc

ip-network-project.out: objects/ip-network-project.o
	$(CC) $^ -o $@ $(LDFLAGS)

objects/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	-rm index.html
	-rm *.out
	-rm objects/*.o
