CC=g++


all: mines

dev: clean mines

mines:
	$(CC) -o mines mines.cpp -lncurses

clean:
	rm -rf mines
