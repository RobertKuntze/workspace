#include "Benchmark.hpp"
#include "connection.hpp"

#include <chrono>
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <vector>


void Benchmark::latencyTestSend(size_t msgSize, size_t iterations)
{
	MMapConnection con;
	con.setup();
	// con.initHeader();

	char* data = new char[msgSize];
	memset(data, 0, msgSize);

	for (int i = 0; i < iterations; i++) {
		con.send(data, msgSize);
		auto time = std::chrono::high_resolution_clock::now();
		con.send(&time, sizeof(time));
	}
}

void Benchmark::LatencyTestReceive(size_t iterations)
{
	MMapConnection con;
	con.setup();
	
	size_t i = 2;
	size_t size = -1;
	std::chrono::_V2::high_resolution_clock::time_point send_time;
	std::vector<std::chrono::nanoseconds> latencies;

	while (true)
	{
		unsigned long offset = con.receive(i, size);
		if (offset >= 0 && size == sizeof(send_time)) {
			memcpy(&send_time, con.mmap_ptr+offset, size);
			auto end_time = std::chrono::high_resolution_clock::now();
			latencies.push_back(end_time - send_time);
			i += 2;
			if (i >= iterations*2 - 1) {
				break;
			}
		}
		size = -1;
	}

	std::chrono::nanoseconds average = std::accumulate(latencies.begin(), latencies.end(), std::chrono::nanoseconds(0)) / latencies.size();

	std::chrono::_V2::high_resolution_clock::time_point start_time;
	memcpy(&start_time, con.mmap_ptr + con.header->DAT[2].offset, sizeof(start_time));
	auto total_time = std::chrono::high_resolution_clock::now() - start_time;

	std::cout << "Average Latency: " << average.count() << " nanoseconds" << std::endl;
	
	// for (int j = 0; j < latencies.size(); j++) {
	// 	std::cout << latencies[j] << std::endl;
	// }

	std::cout << "Total Time: " << total_time.count() / 1e9 << " seconds" << std::endl;
	std::cout << "Bandwidth: " << 1.0 / (total_time.count() / 1e9) << " GB/s" << std::endl;

	con.cleanup();
}