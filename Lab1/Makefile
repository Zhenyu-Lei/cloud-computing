CXFLAGS+=-O2 -ggdb -DDEBUG
CXXFLAGS+=-Wall -Wextra

all: sudoku_solve

sudoku_solve: main.cpp neighbor.cpp sudoku_dancing_links.cpp
	g++ -O2 -o $@ $^ -lpthread

clean:
	rm sudoku_solve
