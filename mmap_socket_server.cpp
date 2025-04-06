#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/mysocket"

int main()
{
    std::cout << "Host online\n";

    const char* filePath = "/dev/shm/mmap_message";

    remove(filePath);

    int fd = open(filePath, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        std::cerr << "Fehler beim Öffnen der Datei" << std::endl;
        return 1;
    }

    //file size
    const size_t filesize = 1024 * 1024 * 1024; // 1 GB
    ftruncate(fd, filesize);


    // create Memory Mapped file
    const int offset = 0;
    void* fileMemory = mmap(nullptr, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    if (fileMemory == MAP_FAILED) {
        std::cerr << "Fehler beim Abbilden der Datei in den Speicher" << std::endl;
        close(fd);
        return 1;
    }

    // create server file descriptor for socket
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    // ??
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strcpy(addr.sun_path, SOCKET_PATH);
    unlink(SOCKET_PATH);

    // bind fd to socket addr
    bind(server_fd, (sockaddr*)&addr, sizeof(addr));

    // start listening to socket, 1 connection only
    listen(server_fd, 1);

    // wait for and accept connection
    int client_fd = accept(server_fd, nullptr, nullptr);

    std::cout << "Accepted Connection\n";

    int buf[100];
    char out_msg[255];
    
    while (true) {
        // read from socket
        int n = read(client_fd, buf, sizeof(buf));
        if (n > 0) {
            std::cout << "Received: " << buf << std::endl;
        }

        char* msg = static_cast<char*>(fileMemory);
        std::cout << "Message received: " << msg << std::endl;

        std::cout << "Write back: ";
        std::cin >> out_msg;

        int messageSize = sizeof(out_msg);
        std::memcpy(fileMemory, out_msg, messageSize);
        std::cout << "Copied to mmap file\n";

        write(client_fd, &messageSize, sizeof(messageSize));
        std::cout << "Wrote to socket\n";
    }

    std::cout << "Host offline\n";

    munmap(fileMemory, filesize);
    close(fd);
    remove(filePath);

    return 0;
}