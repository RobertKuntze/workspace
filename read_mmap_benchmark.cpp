#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <chrono>
#include <iostream>

#define SOCKET_PATH "/tmp/mysocket"

int main() {
    const char* filePath = "/dev/shm/mmap_message";

    // Datei öffnen
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        std::cerr << "Fehler beim Öffnen der Datei" << std::endl;
        return 1;
    }

    // Dateigröße ermitteln
    struct stat fileInfo;
    if (fstat(fd, &fileInfo) == -1) {
        std::cerr << "Fehler beim Abrufen der Dateiinformationen" << std::endl;
        close(fd);
        return 1;
    }

    // Datei in den Speicher abbilden
    void* fileMemory = mmap(nullptr, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (fileMemory == MAP_FAILED) {
        std::cerr << "Fehler beim Abbilden der Datei in den Speicher" << std::endl;
        close(fd);
        return 1;
    }

    const size_t filesize = 1UL << 30; // 1GB

    while (true) {
        char* ready = static_cast<char*>(fileMemory);
        if (*ready == 0x42) { break; }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto end_ns = std::chrono::duration_cast<std::chrono::microseconds>(end.time_since_epoch());
    long end_value = end_ns.count();

    long long* start = static_cast<long long*>(fileMemory + 1);
    long long start_value = *start;

    std::cout << "Bandwidth: " << end_value - start_value << "GB/s" << std::endl;

    return 0;
}