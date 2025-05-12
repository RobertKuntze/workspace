#include "Benchmark.hpp"
#include <iostream>
#include <cstring>
#include <stddef.h>

const size_t iter = 1024;
const size_t size = 1 << 22;

int main(int argc, char const *argv[])
{
	Benchmark bench;

	if (argc == 1) {
		std::cout << "no arguments" << std::endl;
		return -1;
	}

	if (!std::strcmp(argv[1], "w")) {
		bench.latencyTestSend(size, iter);
	}

	if(!std::strcmp(argv[1], "b")) {
		for (size_t i = 1UL << 22; i < 1UL << 27; i <<= 1) {
			for (size_t j = 1; j <= 8; j++) {
				bench.sendBandwidthTest(5, i, j);
			}
		}
	}

	if (!std::strcmp(argv[1], "r")) {
		bench.LatencyTestReceive(iter);
	}
 
	return 0;
}
