#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <chrono>
#include <ratio>

#include "write_mmap_benchmark.hpp"

#define SOCKET_PATH "/tmp/mysocket"
#define DONE 0x42
#define FILESIZE 1UL << 30 // 1 GB

int main()
{
    
    const char* filePath = "/dev/shm/mmap_message";
    
    int fd = open(filePath, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        std::cerr << "Fehler beim Öffnen der Datei" << std::endl;
        return 1;
    }
    
    //file size
    ftruncate(fd, FILESIZE);

    // create Memory Mapped file
    void* fileMemory = mmap(nullptr, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fileMemory == MAP_FAILED) {
        std::cerr << "Fehler beim Abbilden der Datei in den Speicher" << std::endl;
        close(fd);
        return 1;
    }
	
	unsigned long bufferSize = 1UL << 20;
    char buffer[bufferSize];
    memset(buffer, 0x01, bufferSize);
	
    auto start = std::chrono::high_resolution_clock::now();
    auto start_duration = std::chrono::duration_cast<std::chrono::microseconds>(start.time_since_epoch());
    long long start_ms = start_duration.count();
	char start_buffer[sizeof(start_ms)];

	memcpy(start_buffer, &start_ms, sizeof(start_ms));
    
	long long off = 0x10;
	memcpy(fileMemory + 8, &off, 8);
	for (int i = 0; i < 1000; i++) {
		writeToMMap(fileMemory, buffer, sizeof(buffer));
	}
	
	writeToMMap(fileMemory, start_buffer, sizeof(start_buffer));
	memset(fileMemory, DONE, 8);
	
    munmap(fileMemory, FILESIZE);
    close(fd);
    remove(filePath);

    return 0;
}

void writeToMMap(void* fileMemory, char data[], long size) {
	long long* offset = static_cast<long long*>(fileMemory + 8);
	if (*offset + size > FILESIZE) {
		std::cerr << "Data too big for file" << std::endl;
		return;
	}

	memcpy(fileMemory + *offset, data, size);

	*offset += size;
	memcpy(fileMemory + 8, offset, sizeof(offset));
}