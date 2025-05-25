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

void Benchmark::sendBandwidthTest(size_t length_seconds, size_t msg_size)
{
	// std::cout << "Starting send Bandwidth test with " << (double) msg_size / (1UL << 20) << " MB per message and " << con->num_threads << " threads" << std::endl;
	std::cout << "{\n";
	std::cout << "  \"type\": \"send\",\n";
	std::cout << "  \"size\": " << msg_size <<  ",\n";
	std::cout << "  \"duration\": " << length_seconds <<  ",\n";
	std::cout << "  \"threads\": " << con->num_threads <<  ",\n";

	con->initHeader();
	
	con->header->send_ready.store(true);
	while(!con->header->receive_ready.load()) {};
	
	size_t iterations = 0;
	
	char* data = new char[msg_size];
	memset(data, 0x69, msg_size);
	
	auto start_time = std::chrono::steady_clock::now();
	auto end_time = std::chrono::steady_clock::now();
	while (true)
	{
		if ((end_time - start_time).count() >= length_seconds * 1e9) {
			break;
		}
		con->send(data, msg_size);
		iterations++;
		end_time = std::chrono::steady_clock::now();
	}
	
	con->header->send_ready.store(false);

	auto duration_ns = (end_time - start_time).count();
	double bandwidth_gbps = (msg_size * iterations) / static_cast<double>(duration_ns);

	std::cout << "  \"total_time_ns\": " << duration_ns << ",\n";
	std::cout << "  \"iterations\": " << iterations << ",\n";
	std::cout << "  \"bandwidth_gbps\": " << bandwidth_gbps << "\n";
	std::cout << "},\n";

	// for (size_t i = 1; i <= 10; i++)
	// {
	// 	std::cout << "DAT[" << i << "]: " << "Offset: " << con->header->DAT[i].offset << " - Size: " << con->header->DAT[i].size << " char: " << *(char*)(con->mmap_ptr + con->header->DAT[i].offset) << std::endl;
	// }
	
	while (con->header->receive_ready.load()) {};
	
}

void Benchmark::receiveBandwidthTest(size_t msg_size) 
{
	std::cout << "{\n";
	std::cout << "  \"type\": \"receive\",\n";
	std::cout << "  \"size\": " << msg_size <<  ",\n";
	std::cout << "  \"threads\": " << con->num_threads <<  ",\n";

	while(!con->header->send_ready.load()) {};
	con->header->receive_ready.store(true);

	char* receive_Buffer = new char[msg_size];
	
	// char* expected = new char[msg_size];
	// memset(expected, 'i', msg_size);

	size_t iterations = 0;
	
	auto start_time = std::chrono::steady_clock::now();
	while (true)
	{
		if (con->header->read_seq.load() != con->header->write_seq) {
			con->receive(receive_Buffer);
			// if(memcmp(expected, receive_Buffer, msg_size) != 0) {
			// 	std::cout << "RECEIVE ERROR" << std::endl;
			// }
			iterations++;
		} else {
			if (!con->header->send_ready.load()) {
				break;
			}
		}
	}
	
	auto end_time = std::chrono::steady_clock::now();

	auto duration_ns = (end_time - start_time).count();
	double bandwidth_gbps = (msg_size * iterations) / static_cast<double>(duration_ns);

	std::cout << "  \"total_time_ns\": " << duration_ns << ",\n";
	std::cout << "  \"iterations\": " << iterations << ",\n";
	std::cout << "  \"bandwidth_gbps\": " << bandwidth_gbps << "\n";
	std::cout << "},\n";

	con->header->receive_ready.store(false);
}

void Benchmark::sendLatencyTest(size_t msg_size, size_t iterations)
{
	con->initHeader();
	char* data = new char[msg_size];

	std::vector<std::chrono::steady_clock::time_point> timepoints;

	con->header->send_ready.store(true);
	while(!con->header->receive_ready.load()) {};

	for (size_t i = 0; i < iterations; i++) {
		
		timepoints.emplace_back(std::chrono::steady_clock::now());
		con->send(data, msg_size);

		// timepoints.emplace_back(std::chrono::steady_clock::now());
	}

	con->send(timepoints.data(), iterations * sizeof(std::chrono::steady_clock::time_point));

	con->header->send_ready.store(false);
	while (con->header->receive_ready.load()) {};
}

void Benchmark::receiveLatencyTest(size_t msg_size, size_t iterations)
{
	std::cout << "{\n";
	std::cout << "  \"type\": \"receive_latency\",\n";
	std::cout << "  \"size\": " << msg_size <<  ",\n";
	std::cout << "  \"threads\": " << con->num_threads <<  ",\n";

	std::vector<std::chrono::steady_clock::time_point> timepoints;

	char* data = new char[msg_size];

	while(!con->header->send_ready.load()) {};
	con->header->receive_ready.store(true);

	for (size_t i = 0; i < iterations; i++) {
		while(con->header->read_seq.load() == con->header->write_seq) {}
		timepoints.emplace_back(std::chrono::steady_clock::now());
		con->receive(data);
	}

	std::vector<std::chrono::steady_clock::time_point> start_timepoints;

	start_timepoints.resize(iterations);

	while (con->header->read_seq.load() == con->header->write_seq) {}
	con->receive(start_timepoints.data());

	for (size_t i=0; i < iterations; i++) {
		std::cout << (timepoints[i] - start_timepoints[i]).count() << std::endl;
	}

	con->header->receive_ready.store(false);
}