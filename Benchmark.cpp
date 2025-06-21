#include "Benchmark.hpp"
#include "connection.hpp"

#include <chrono>
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <vector>
#include <random>

size_t seed = 6969420;

extern MMapConnection* con;
extern size_t PACKET_SIZE;

void Benchmark::sendBandwidthTest(size_t length_seconds, size_t msg_size)
{
	// // std::cout << "Starting send Bandwidth test with " << (double) msg_size / (1UL << 20) << " MB per message and " << con->num_threads << " threads" << std::endl;
	// std::cout << "{\n";
	// std::cout << "  \"type\": \"send\",\n";
	// std::cout << "  \"size\": " << msg_size <<  ",\n";
	// std::cout << "  \"duration\": " << length_seconds <<  ",\n";
	// std::cout << "  \"threads\": " << con->num_threads <<  ",\n";

	// con->initHeader();
	
	// con->header->send_ready.store(true);
	// while(!con->header->receive_ready.load()) {};
	
	// size_t iterations = 0;
	
	// size_t data_size = msg_size / 8;
	// uint64_t* data = new uint64_t[data_size];
	// // memset(data, 0x69, msg_size);

    // std::mt19937_64 gen(seed);
    // std::uniform_int_distribution<uint64_t> dis;

    // for (size_t i = 0; i < data_size; ++i) {
    //     data[i] = dis(gen);
    // }

	// DataObject object{msg_size, };
	
	// auto start_time = std::chrono::steady_clock::now();
	// auto end_time = std::chrono::steady_clock::now();
	// while (true)
	// {
	// 	if ((end_time - start_time).count() >= length_seconds * 1e9) {
	// 		break;
	// 	}
	// 	con->send(data, msg_size);
	// 	iterations++;
	// 	end_time = std::chrono::steady_clock::now();
	// }
	// con->completeAllMessages();
	// end_time = std::chrono::steady_clock::now();
	
	// con->header->send_ready.store(false);

	// auto duration_ns = (end_time - start_time).count();
	// double bandwidth_gbps = (msg_size * iterations) / static_cast<double>(duration_ns);

	// std::cout << "  \"total_time_ns\": " << duration_ns << ",\n";
	// std::cout << "  \"iterations\": " << iterations << ",\n";
	// std::cout << "  \"bandwidth_gbps\": " << bandwidth_gbps << "\n";
	// std::cout << "}\n";

	// // for (size_t i = 1; i <= 10; i++)
	// // {
	// // 	std::cout << "DAT[" << i << "]: " << "Offset: " << con->header->DAT[i].offset << " - Size: " << con->header->DAT[i].size << " char: " << *(char*)(con->mmap_ptr + con->header->DAT[i].offset) << std::endl;
	// // }
	
	// while (con->header->receive_ready.load()) {};
	
}

void Benchmark::receiveBandwidthTest(size_t msg_size) 
{
	std::cout << "{\n";
	std::cout << "  \"type\": \"receive\",\n";
	std::cout << "  \"size\": " << msg_size <<  ",\n";
	std::cout << "  \"threads\": " << con->num_threads <<  ",\n";

	while(!con->header->send_ready.load()) {};
	con->header->receive_ready.store(true);

	
	size_t data_size = msg_size / 8;
	uint64_t* data = new uint64_t[data_size];
	// memset(data, 0x69, msg_size);
	
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<uint64_t> dis;
	
    for (size_t i = 0; i < data_size; ++i) {
		data[i] = dis(gen);
	}
	
	size_t iterations = 0;
	
	auto start_time = std::chrono::steady_clock::now();
	DataObject receive_buffer{new char[msg_size], msg_size, iterations};
	
	con->receive(&receive_buffer);
	
	while (!(receive_buffer.total_size == receive_buffer.transferred_size)) {}
	// if(memcmp(data, receive_buffer.data, msg_size) != 0) {
	// 	std::cout << "RECEIVE ERROR" << std::endl;
	// }
	iterations++;
	auto end_time = std::chrono::steady_clock::now();

	auto duration_ns = (end_time - start_time).count();
	double bandwidth_gbps = (msg_size * iterations) / static_cast<double>(duration_ns);

	std::cout << "  \"total_time_ns\": " << duration_ns << ",\n";
	std::cout << "  \"iterations\": " << iterations << ",\n";
	std::cout << "  \"bandwidth_gbps\": " << bandwidth_gbps << "\n";
	std::cout << "}\n";

	con->header->receive_ready.store(false);
}

void printvector(const std::vector<std::chrono::steady_clock::time_point>& vec) {
	std::cout << " [";
	for (size_t i = 0; i < vec.size(); ++i) {
		std::cout << vec[i].time_since_epoch().count();
		if (i != vec.size() - 1)
			std::cout << ",";
	}
	std::cout << "]";
}
void Benchmark::sendLatencyTest(size_t msg_size, size_t iterations)
{
	con->initHeader();
	
	
	std::cout << "{\n";
	std::cout << "  \"type\": \"send_latency\",\n";
	std::cout << "  \"size\": " << msg_size <<  ",\n";
	std::cout << "  \"threads\": " << con->num_threads <<  ",\n";
	std::cout << "  \"packet size\": " << PACKET_SIZE <<  ",\n";
	
	
	std::vector<std::chrono::steady_clock::time_point> t1;
	std::vector<std::chrono::steady_clock::time_point> t2;
	
	con->header->send_ready.store(true);
	while(!con->header->receive_ready.load()) {};
	
	for (size_t i = 0; i < iterations; i++) {
		DataObject* object = new DataObject{new char[msg_size], msg_size, i};
		
		t1.emplace_back(std::chrono::steady_clock::now());
		con->send(object);
		t2.emplace_back(std::chrono::steady_clock::now());
	}
	// einfach hier ausgeben
	std::cout << "  \"t1\": ";
	printvector(t1);
	std::cout << ",\n";
	std::cout << "  \"t2\": ";
	printvector(t2);

	std::cout << "}\n";

	con->header->send_ready.store(false);
	while (con->header->receive_ready.load()) {};
}


void Benchmark::receiveLatencyTest(size_t msg_size, size_t iterations)
{
	std::cout << "{\n";
	std::cout << "  \"type\": \"receive_latency\",\n";
	std::cout << "  \"size\": " << msg_size <<  ",\n";
	std::cout << "  \"threads\": " << con->num_threads <<  ",\n";

	std::vector<std::chrono::steady_clock::time_point> t3;
	std::vector<std::chrono::steady_clock::time_point> t4;

	while(!con->header->send_ready.load()) {};
	con->header->receive_ready.store(true);

	for (size_t i = 0; i < iterations; i++) {
		DataObject* object = new DataObject{new char[msg_size], msg_size, i};
		t3.emplace_back(std::chrono::steady_clock::now());
		con->receive(object);
		t4.emplace_back(std::chrono::steady_clock::now());
	}


	std::cout << "  \"t3\": ";
	printvector(t3);
	std::cout << ",\n";
	std::cout << "  \"t4\": ";
	printvector(t4);
	std::cout << "\n";

	std::cout << "}\n";
	
	con->header->receive_ready.store(false);
}

void Benchmark::sendBandwidthTotalSizeTest(uint64_t total_size)
{
	std::cout << "{\n";
	std::cout << "  \"type\": \"s_bw_t\",\n";
	std::cout << "  \"total data sent\": " << total_size <<  ",\n";
	std::cout << "  \"threads\": " << con->num_threads <<  ",\n";

	con->initHeader();
	
	con->header->send_ready.store(true);
	while(!con->header->receive_ready.load()) {};

	
	// memset(data, 0x69, msg_size);
	size_t data_size = total_size / sizeof(uint64_t);
	uint64_t* data = new uint64_t[data_size];

    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<uint64_t> dis;

    for (size_t i = 0; i < (total_size / sizeof(uint64_t)); ++i) {
        data[i] = dis(gen);
    }

	DataObject* obj = new DataObject{(char*) data, total_size, 0};

	auto start_time = std::chrono::steady_clock::now();
	con->send(obj);
	auto end_time = std::chrono::steady_clock::now();
	
	con->header->send_ready.store(false);

	auto duration_ns = (end_time - start_time).count();
	double bandwidth_gbps = (total_size) / static_cast<double>(duration_ns);

	std::cout << "  \"total_time_ns\": " << duration_ns << ",\n";
	std::cout << "  \"bandwidth_gbps\": " << bandwidth_gbps << "\n";
	std::cout << "}\n";

	// for (size_t i = 1; i <= 10; i++)
	// {
	// 	std::cout << "DAT[" << i << "]: " << "Offset: " << con->header->DAT[i].offset << " - Size: " << con->header->DAT[i].size << " char: " << *(char*)(con->mmap_ptr + con->header->DAT[i].offset) << std::endl;
	// }
	
	while (con->header->receive_ready.load()) {};
}