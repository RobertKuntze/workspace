#include "Benchmark.hpp"
#include <iostream>
#include <cstring>

const int iter = 1024;

int main(int argc, char const *argv[])
{
	Benchmark bench;

	if (argc == 1) {
		std::cout << "no arguments" << std::endl;
		return -1;
	}

	if (!std::strcmp(argv[1], "w")) {
		bench.latencyTestSend(1 << 20, iter);
	}

	if (!std::strcmp(argv[1], "r")) {
		bench.LatencyTestReceive(iter);
	}
 
	return 0;
}
