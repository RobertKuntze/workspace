#include "connection.hpp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <cstring>


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

	sleep(1);

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
	header->status = true;
	if (header->index >= HEADER_DAT_SIZE) {
		return;
	}

	DataAccessEntry cur = header->DAT[header->index];
	unsigned long long offset = cur.offset + cur.size;


	if (offset + size > MMAP_FILESIZE) {
		return;
	}

	memcpy(mmap_ptr + offset, data, size);

	header->index++;
	header->DAT[header->index] = DataAccessEntry{(unsigned int) offset,(unsigned int) size};
}

long MMapConnection::receive(unsigned long read_index, size_t& size)
{
	if (!header->status) {
		return -1;
	}
	if (read_index <= header->index) {
		DataAccessEntry cur = header->DAT[read_index];
		char data[cur.size];
		memcpy(&data, mmap_ptr + cur.offset, cur.size);
		read_index++;
		size = cur.size;
		return cur.offset;
	}

	return -1;
}