#include "Benchmark.hpp"
#include "connection.hpp"

#include <iostream>
#include <cstring>
#include <stddef.h>
#include <thread>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <cstring>
#include <chrono>
#include <string>
#include <iostream>

const size_t iter = 10240;
size_t size = 1 << 22;
size_t num_threads = 16;

MMapConnection* con = new MMapConnection();
Benchmark bench;

int main(int argc, char const *argv[])
{
	con->setup(true, num_threads);

	if (argc == 1) {
		std::cout << "no arguments" << std::endl;
		return -1;
	}

	if (!std::strcmp(argv[1], "w")) {
		bench.latencyTestSend(size, iter);
	}

	if(!std::strcmp(argv[1], "b")) {
		for (; size < 1 << 27; size *= 2) {
			bench.sendBandwidthTest(10, size, num_threads);
		}
	}

	if (!std::strcmp(argv[1], "r")) {
		bench.LatencyTestReceive(iter);
	}

	con->cleanup();
 
	return 0;
}
