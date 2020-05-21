
CC=gcc

CFLAGS=-O3 -Wall -g -pg

LIBS=-lcrypto

all : assetserver gfas ghas

assetserver : readfile.o writefile.o assetserver.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

gfas : readfile.o writefile.o gfas.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

ghas : readfile.o writefile.o ghas.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
