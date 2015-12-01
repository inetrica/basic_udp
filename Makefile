CC = gcc
CFLAGS = -Wall -Werror
BR = blaster
BE = blastee
EXE = $(BR) $(BE)


all: $(BR) $(BE)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BR): $(BR).o 
	$(CC) -o $@ $^ -lrt $(CFLAGS)

$(BE): $(BE).o
	$(CC) -o $@ $^ -lrt $(CFLAGS)

clean:
	rm -f $(EXE) *.o 
