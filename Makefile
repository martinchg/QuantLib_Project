
.PHONY: all build test clean

all: build test

build: main

test: main
	./main

main: *.hpp *.cpp
	g++ *.cpp -I/opt/homebrew/include `quantlib-config --cflags` -O3 `quantlib-config --libs` -L/opt/homebrew/lib -o main

clean:
	rm -f main
