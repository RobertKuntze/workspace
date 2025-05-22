#include "Benchmark.hpp"
#include "connection.hpp"

#include <chrono>
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <vector>

extern MMapConnection* con;


void Benchmark::latencyTestSend(size_t msgSize, size_t iterations)
{
	MMapConnection con;
	con.setup(true, 8);
	// con.initHeader();

	// char* data = new char[msgSize];
	// memset(data, 0, msgSize);

	for (size_t i = 0; i < iterations; i++) {
		auto time = std::chrono::high_resolution_clock::now();
		// con.send(data, msgSize, 8);
		con.send(&time, sizeof(time), 1);
	}
}

void Benchmark::LatencyTestReceive(size_t iterations)
{
	MMapConnection con;
	con.setup(false, 1);

	std::cout << "Starting Latency Receive Test" << std::endl;
	
	size_t i = 0;
	size_t size = 0;
	std::chrono::_V2::high_resolution_clock::time_point send_time;
	std::vector<std::chrono::nanoseconds> latencies;

	while (true)
	{
		void* ptr = con.receive(i, size);
		if (ptr && size == sizeof(send_time)) {
			memcpy(&send_time, ptr, size);
			auto end_time = std::chrono::high_resolution_clock::now();
			latencies.push_back(end_time - send_time);
			i += 2;
			if (i >= iterations*2 - 1) {
				break;
			}
		}
		size = 0;
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
	// std::cout << "Bandwidth: " << 4.0 / (total_time.count() / 1e9) << " GB/s" << std::endl;
}

void Benchmark::sendBandwidthTest(size_t length_seconds, size_t msg_size, size_t num_threads)
{
	std::cout << "Starting send Bandwidth test with " << (double) msg_size / (1UL << 20) << " MB per message and " << num_threads << " threads" << std::endl;

	auto start_time = std::chrono::high_resolution_clock::now();
	auto end_time = std::chrono::high_resolution_clock::now();

	size_t iterations = 0;

	char* data = new char[msg_size];
	memset(data, 0x69, msg_size);

	while (true)
	{
		if ((end_time - start_time).count() >= length_seconds * 1e9) {
			break;
		}
		con->send(data, msg_size);
		iterations++;
		end_time = std::chrono::high_resolution_clock::now();
	}

	std::cout << "Total Time: " << (end_time - start_time).count() << std::endl;
	std::cout << "Bandwidth: " << (msg_size * iterations) / (double) (end_time - start_time).count() << "GB/s" << std::endl;
}