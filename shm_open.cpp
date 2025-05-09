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

	if (ftruncate(fd_h, sizeof(Header))) {
		std::cerr << "ftruncate Error Header" << std::endl;
	}
	
	if (ftruncate(fd_d, DATA_FILE_SIZE)) {
		std::cerr << "ftruncate Error Data" << std::endl;
	}

	Header* header = (Header *) mmap(nullptr, sizeof(Header), PROT_READ | PROT_WRITE, MAP_SHARED, fd_h, 0);

	new (header) Header();

	void* data_file = mmap(nullptr, DATA_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_d, 0);
	char data[BLOCK_SIZE];
	memset(data, 0, sizeof(data));

	auto start = std::chrono::high_resolution_clock::now();
	
	for (int i = 0; i < (DATA_FILE_SIZE / BLOCK_SIZE) - 1; i++) {
		header->sendData(data_file, &data, sizeof(data));
	}
	
	auto start_duration = std::chrono::duration_cast<std::chrono::microseconds>(start.time_since_epoch());
	long long start_ms = start_duration.count();
	header->sendData(data_file, &start_ms, sizeof(start_ms));

	header->status = 0x69;

	munmap(header, sizeof(Header));
	munmap(data_file, DATA_FILE_SIZE);
	close(fd_h);
	close(fd_d);

	return 0;
}