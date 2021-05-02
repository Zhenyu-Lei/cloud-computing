CXXFLAGS+=-O2 -ggdb -DDEBUG
CXXFLAGS+=-Wall -Wextra

all: httpserver

httpserver: httpserverMy.cc
	g++ -O2 -o $@ $^ -pthread

clean:
	rm httpserver
