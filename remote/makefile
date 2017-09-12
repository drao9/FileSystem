CC = gcc 

CFLAGS = -g

EXECUTABLES = server client

all: $(EXECUTABLES)

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c

client: client.o sockettome.o
	$(CC) $(CFLAGS) -o client client.o sockettome.o

server: server.o sockettome.o
	$(CC) $(CFLAGS) -o server server.o sockettome.o

clean:
	rm *.o $(EXECUTABLES)