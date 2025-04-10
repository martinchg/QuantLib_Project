
.PHONY: all build test clean

all: build test

build: main

test: main
	./main

main: *.hpp *.cpp
	g++ *.cpp `quantlib-config --cflags` -g0 -O3 `quantlib-config --libs` -o main

clean:
	rm -f main
