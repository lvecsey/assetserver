
CC=gcc

CFLAGS=-O3 -Wall -g -pg

all : assetserver gfas

assetserver : readfile.o writefile.o assetserver.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

gfas : readfile.o writefile.o gfas.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
