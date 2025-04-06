#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SOCKET_PATH "/tmp/mysocket"

int main() {
    const char* filePath = "/dev/shm/mmap_message";

    // Datei öffnen
    int fd = open(filePath, O_RDWR);
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
    void* fileMemory = mmap(nullptr, fileInfo.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fileMemory == MAP_FAILED) {
        std::cerr << "Fehler beim Abbilden der Datei in den Speicher" << std::endl;
        close(fd);
        return 1;
    }

    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strcpy(addr.sun_path, SOCKET_PATH);

    connect(sock_fd, (sockaddr*)&addr, sizeof(addr));

    std::cout << "Connection succesfull\n";

    std::string msg;

    int buf[100];
    char out_msg[255];
    
    while (true) {
        std::cout << "Write back: ";
        std::cin >> out_msg;
        
        int messageSize = sizeof(out_msg);
        std::cout << "messagesize: " << messageSize << "\nsizeof(messagesize): " << sizeof(messageSize) << std::endl;
        std::memcpy(fileMemory, out_msg, messageSize);
        
        write(sock_fd, &messageSize, sizeof(messageSize));

        int n = read(sock_fd, buf, sizeof(buf));
        if (n > 0) {
            std::cout << "Received: " << buf << std::endl;
        }

        char* msg = static_cast<char*>(fileMemory);
        std::cout << "Message received: " << msg << std::endl;

    }
    close(sock_fd);

    return 0;
}