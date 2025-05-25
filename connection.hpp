#include <stddef.h>
#include <atomic>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <functional>

const char HEADERPATH[] = "/header";
const char FILEPATH[] = "/file";
const unsigned long long MMAP_FILESIZE = 1ULL << 32;
const unsigned long HEADER_DAT_SIZE = 1UL << 11;

struct DataAccessEntry {
	uint64_t offset;
	uint64_t size;
};

struct Header {
	bool status;
	std::atomic<size_t> write_seq;
	std::atomic<size_t> read_seq;
	DataAccessEntry DAT[HEADER_DAT_SIZE];
	std::atomic<bool> send_ready{false};
	std::atomic<bool> receive_ready{false};

	Header() {
		status = 0;
		write_seq = 0;
		read_seq = 0;
		DAT[0] = {DataAccessEntry{}};
	}
};

class Connection
{
public:
	// virtual void send(void* data, size_t size) = 0;
	virtual void send(void* data, size_t size, size_t num_threads) = 0;
	virtual void send(void* data, size_t size) = 0;
	virtual void* receive(void* receive_buffer_) = 0;
	virtual void setup(bool cleanInit, size_t num_threads) = 0;
	virtual void cleanup() = 0;
	virtual ~Connection() = default;
};

class MMapConnection : public Connection
{
public:
	// void send(void* data, size_t size);
	void send(void* data, size_t size, size_t num_threads);
	void send(void* data, size_t size);
	void* receive(void* receive_buffer_);
	void setup(bool cleanInit, size_t num_threads);
	void initHeader();
	// void initMMap();
	void cleanup();

	Header* header;
	void* mmap_ptr;
	
// private:
	int fd_h, fd;
	size_t num_threads;

	std::atomic<bool> stop_flag{false};
	std::vector<std::unique_ptr<std::condition_variable>> cv_s;
	std::vector<std::unique_ptr<std::condition_variable>> cv_r;
	std::vector<std::unique_ptr<std::mutex>> mtx_s;
	std::vector<std::unique_ptr<std::mutex>> mtx_r;
	std::vector<std::unique_ptr<std::thread>> send_threads;
	std::vector<std::unique_ptr<std::thread>> receive_threads;

	std::unique_ptr<std::mutex> cout_mut = std::make_unique<std::mutex>();

	std::vector<void*> data_ptrs;
	std::vector<size_t> data_sizes;
	std::vector<std::unique_ptr<std::atomic<bool>>> data_ready;
	std::vector<uint64_t> target_offsets;

	std::vector<uint64_t> receive_ptrs;
	std::vector<size_t> receive_sizes;
	std::vector<std::unique_ptr<std::atomic<bool>>> receive_ready;
	std::vector<uint64_t> receive_offsets;
	
	char* receive_buffer;

	std::function<void(std::atomic<bool> *, size_t thread, size_t num_threads)> thread_send;
	std::function<void(std::atomic<bool> *, size_t thread, size_t num_threads)> thread_receive;

};

// class MMapConnection2 : public Connection
// {
// public:
// 	void send(void* data, size_t size);
// 	void* receive(unsigned long read_index, size_t& size);
// 	void setup();
// 	void cleanup();
	
// 	std::vector<void*> mmap_buffers;
// 	void* mmap_ptr;
// 	Header* header;
// };