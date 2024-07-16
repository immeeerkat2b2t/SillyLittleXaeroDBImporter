CC = gcc
CFLAGS = -pthread -g
LIBS = -lsqlite3 -lncurses

all: SillyLittleXaeroDBImporter

SillyLittleXaeroDBImporter: src/main.c
	$(CC) $(CFLAGS) -o SillyLittleXaeroDBImporter src/main.c $(LIBS)

clean:
	rm -f SillyLittleXaeroDBImporter
