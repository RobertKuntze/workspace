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

#define SOCKET_PATH "/tmp/mysocket"

int main()
{
    
    const char* filePath = "/dev/shm/mmap_message";
    
    int fd = open(filePath, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        std::cerr << "Fehler beim Öffnen der Datei" << std::endl;
        return 1;
    }
    
    //file size
    const size_t filesize = 1UL << 30; // 1 GB
    ftruncate(fd, filesize);
    
    sleep(5);

    // create Memory Mapped file
    const int offset = 0;
    void* fileMemory = mmap(nullptr, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    if (fileMemory == MAP_FAILED) {
        std::cerr << "Fehler beim Abbilden der Datei in den Speicher" << std::endl;
        close(fd);
        return 1;
    }

    char* buffer = new char[filesize];
    memset(buffer, 0x69, filesize);

    auto start = std::chrono::high_resolution_clock::now();
    auto start_duration = std::chrono::duration_cast<std::chrono::microseconds>(start.time_since_epoch());
    long long start_ms = start_duration.count();
    
    memcpy(fileMemory, buffer, filesize);
    memset(fileMemory, 0x42, 1);
    memcpy(fileMemory + 1, &start_ms, sizeof(start_ms));

    munmap(fileMemory, filesize);
    close(fd);
    remove(filePath);

    return 0;
}