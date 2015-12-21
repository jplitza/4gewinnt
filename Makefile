CC = gcc

ifeq ($(DEBUG),1)
DEFINES = -DDEBUG
else
DEFINES = 
endif

CFLAGS = $(DEFINES)
BIN = bin/gui bin/ai bin/server

all: $(BIN)

bin/gui: gui.o netclient.o net.o gamelogic.o
	$(CC) $(DEFINES) -Wall -o bin/gui gui.c netclient.o net.o gamelogic.o -lm -lcurses

bin/ai: ai.o netclient.o net.o gamelogic.o
	$(CC) $(DEFINES) -Wall -o bin/ai ai.c netclient.o net.o gamelogic.o -lm

bin/server: server.o net.o gamelogic.o
	$(CC) $(DEFINES) -Wall -o bin/server server.c net.o gamelogic.o -lm

clean:
	rm -rf *~ *.ko *.o *.mod.c .*cmd .tmp_versions $(BIN)

ai.o: ai.c
	$(CC) -c ai.c -o ai.o $(CFLAGS)

gamelogic.o: gamelogic.c
	$(CC) -c gamelogic.c -o gamelogic.o $(CFLAGS)

gui.o: gui.c
	$(CC) -c gui.c -o gui.o $(CFLAGS)

net.o: net.c
	$(CC) -c net.c -o net.o $(CFLAGS)

netclient.o: netclient.c
	$(CC) -c netclient.c -o netclient.o $(CFLAGS)

server.o: server.c
	$(CC) -c server.c -o server.o $(CFLAGS)
