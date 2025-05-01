#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <cstring>

#include "Header.hpp"

#define HEADERFILENAME "/header"
#define DATAFILENAME "/data"

int main () 
{
	int fd_h = shm_open(HEADERFILENAME, O_CREAT | O_RDWR, 0666);
	if (fd_h == -1) {
		std::cerr << "shm_open Error Header" << std::endl;
	}

	int fd_d = shm_open(HEADERFILENAME, O_CREAT | O_RDWR, 0666);
	if (fd_d == -1) {
		std::cerr << "shm_open Error Data" << std::endl;
	}

	Header* header = (Header *) mmap(nullptr, sizeof(Header), PROT_READ | PROT_WRITE, MAP_SHARED, fd_h, 0);

	new (header) Header();

	void* data_file = mmap(nullptr, DATA_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_d, 0);

	unsigned long read_index = 0;

	unsigned char data[BLOCK_SIZE];

	std::chrono::_V2::high_resolution_clock::time_point latency;

	while (true) {
		if (read_index < header->index) {
			if (read_index == 0) {
				latency = std::chrono::high_resolution_clock::now();
			}
			DataAccessEntry dae = header->DAT[read_index];
			memcpy(&data, data_file + dae.offset, dae.size);
			read_index++;
		}
		if (header->status == 0x69) {
			break;
		}
	}

	long long start_ms;

	memcpy(&start_ms, data_file + header->DAT[header->index].offset, sizeof(long long));

	auto end = std::chrono::high_resolution_clock::now();
    auto end_ms = std::chrono::duration_cast<std::chrono::microseconds>(end.time_since_epoch());
    long long end_value = end_ms.count();

	auto latency_ms = std::chrono::duration_cast<std::chrono::microseconds>(latency.time_since_epoch());
	long long latency_value = latency_ms.count();

	double total_time_ms = (end_value - start_ms);
	double latency_time_ms = (latency_value - start_ms);

	std::cout << "Total Time: " << total_time_ms << " microseconds" << std::endl;
	std::cout << "Bandwidth: " << (double) (DATA_FILE_SIZE / (1UL << 30))/(total_time_ms / 1e6) << "GB/s" << std::endl;
	std::cout << "Latency: " << latency_time_ms << " microseconds" << std::endl;
	std::cout << "Bandwidth after latency: " << (double) (DATA_FILE_SIZE / (1UL << 30) / ((total_time_ms - latency_time_ms) / 1e6)) << "GB/s" << std::endl; 

	munmap(header, sizeof(Header));
	munmap(data_file, DATA_FILE_SIZE);
	close(fd_h);
	close(fd_d);

	return 0;
}