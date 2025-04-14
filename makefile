all: p1 p2
	~/workspace/p1 &
	~/workspace/p2

gdb-p1: p1-g
	gdb ~/workspace/p1-g

gdb-p2: p2-g
	gdb ~/workspace/p2-g

p1: write_mmap_benchmark.cpp
	g++ -o p1 write_mmap_benchmark.cpp

p2: read_mmap_benchmark.cpp
	g++ -o p2 read_mmap_benchmark.cpp

p1-g: write_mmap_benchmark.cpp
	g++ -g -o p1-g write_mmap_benchmark.cpp

p2-g: read_mmap_benchmark.cpp
	g++ -g -o p2-g read_mmap_benchmark.cpp