#include "connection.hpp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>


void MMapConnection::setup()
{
	int fd_h = shm_open(HEADERPATH, O_CREAT | O_RDWR, 0666);
	if (fd_h == -1) {
		std::cerr << "shm_open Error Header" << std::endl;
	}

	ftruncate(fd_h, 0);
	if (ftruncate(fd_h, sizeof(Header))) {
		std::cerr << "ftruncate Error Header" << std::endl;
	}

	int fd = shm_open(FILEPATH, O_CREAT | O_RDWR, 0666);
	if (fd == -1) {
		std::cerr << "shm_open Error Data" << std::endl;
	}

	ftruncate(fd, 0);
	if (ftruncate(fd, MMAP_FILESIZE)) {
		std::cerr << "ftruncate Error Data" << std::endl;
	}

	header = (Header*) mmap(nullptr, sizeof(Header), PROT_READ | PROT_WRITE, MAP_SHARED, fd_h, 0);

	mmap_ptr = mmap(nullptr, MMAP_FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	// sleep(1);

	close(fd);
	close(fd_h);
}

void MMapConnection::initHeader()
{
	new (header) Header();
}

void MMapConnection::cleanup()
{
	munmap(header, sizeof(Header));
	munmap(mmap_ptr, MMAP_FILESIZE);
}

void MMapConnection::send(void* data, size_t size)
{
	// header->status = true;
	// memcpy(mmap_ptr + offset, data, size);
}

void MMapConnection::send(void* data, size_t size, size_t num_threads)
{
	header->status = true;
	if (header->index >= HEADER_DAT_SIZE) {
		header->index=0;
	}
	
	DataAccessEntry cur = header->DAT[header->index];
	unsigned long long offset = cur.offset + cur.size;

	if (offset + size > MMAP_FILESIZE) {
		std::cout << "Filesize exceeded" << std::endl;
		return;
	}

	std::vector<std::thread> threads;
	size_t block_size = size / num_threads;
	size_t remainder = size % num_threads;

	auto thread_send = [](void* dest, void* data, size_t size) {
		memcpy(dest, data, size);
	};

	for (int i = 0; i < num_threads; i++) {
		size_t off = i * block_size;
		size_t cur_size = i == num_threads - 1 ? block_size + remainder : block_size;

		if (offset + off > MMAP_FILESIZE) {
			std::cout << "FILESIZE EXCEEDED" << std::endl;
		}
		threads.emplace_back(thread_send, mmap_ptr + offset + off, data + off, cur_size);
	}

	for (auto& t : threads) {
		t.join();
	}
	
	header->index++;
	header->DAT[header->index] = DataAccessEntry{(unsigned int) offset,(unsigned int) size};
}

void* MMapConnection::receive(unsigned long read_index, size_t& size)
{	
	if (!header->status) {
		return 0;
	}
	if (read_index <= header->index) {
		DataAccessEntry cur = header->DAT[read_index];
		char data[cur.size];
		memcpy(&data, mmap_ptr + cur.offset, cur.size);
		read_index++;
		size = cur.size;
		return mmap_ptr + cur.offset;
	}

	return 0;
}

void MMapConnection2::setup()
{
	int fd_h = shm_open(HEADERPATH, O_CREAT | O_RDWR, 0666);
	if (fd_h == -1) {
		std::cerr << "shm_open Error Header" << std::endl;
	}

	if (ftruncate(fd_h, 0) || ftruncate(fd_h, sizeof(Header))) {
		std::cerr << "ftruncate Error Header" << std::endl;
	}

	header = (Header*) mmap(nullptr, sizeof(Header), PROT_READ | PROT_WRITE, MAP_SHARED, fd_h, 0);

	close(fd_h);
}

void MMapConnection2::cleanup()
{
	for (size_t i = 0; i < mmap_buffers.size(); i++)
	{
		munmap(mmap_buffers[i], header->DAT[i].size);
	}
	munmap(header, sizeof(Header));
}

// void MMapConnection2::send(void* data, size_t size)
// {
// 	header->status = true;
// 	if (header->index >= HEADER_DAT_SIZE) {
// 		return;
// 	}

// 	unsigned long index = header->index;
// 	index++;

// 	auto name = "/" + std::to_string(index);
// 	int fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
// 	if (ftruncate(fd, size)) { return; }

// 	void* mmap_ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
// 	memcpy(mmap_ptr, data, size);

// 	mmap_buffers.push_back(mmap_ptr);
// 	header->DAT[index] = DataAccessEntry{(unsigned int) index, (unsigned int) size};

// 	header->index = index;
// }

// void* MMapConnection2::receive(unsigned long read_index, size_t& size)
// {
// 	DataAccessEntry cur = header->DAT[read_index];

// 	if (!header->status) {
// 		return 0;
// 	}
// 	if (read_index <= header->index) {
// 		auto name = "/" + std::to_string(read_index);
// 		int fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
// 		if (ftruncate(fd, size)) { return 0; }
// 		void* mmap_ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		
// 		DataAccessEntry cur = header->DAT[read_index];
// 		char data[cur.size];
// 		memcpy(&data, mmap_ptr, cur.size);
// 		read_index++;
// 		size = cur.size;
// 		return mmap_ptr;
// 	}

// 	return 0;
// }