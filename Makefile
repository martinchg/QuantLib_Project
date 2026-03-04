
.PHONY: all build test benchmark clean

all: build test

build: main

test: main
	./main

benchmark: bench/extended_tree_cache_benchmark
	./bench/extended_tree_cache_benchmark

main: *.hpp *.cpp
	g++ *.cpp -I/opt/homebrew/include `quantlib-config --cflags` -O3 `quantlib-config --libs` -L/opt/homebrew/lib -o main

bench/extended_tree_cache_benchmark: bench/extended_tree_cache_benchmark.cpp
	g++ bench/extended_tree_cache_benchmark.cpp -I/opt/homebrew/include `quantlib-config --cflags` -O3 `quantlib-config --libs` -L/opt/homebrew/lib -o bench/extended_tree_cache_benchmark

clean:
	rm -f main
