#include <stddef.h>
#include <atomic>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <functional>
#include <queue>

const char HEADERPATH[] = "/header";
const char FILEPATH[] = "/data";
const unsigned long long MMAP_SIZE_PER_THREAD = 1ULL << 23;
const unsigned long HEADER_DAT_SIZE = 1UL << 7;

struct DataAccessEntry {
	uint64_t offset;
	uint64_t size;
	bool ready;
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
		DAT[0] = {0,0,false};
	}
};

struct DataObject {
	char* data;

	uint64_t total_size;
	uint64_t transferred_size;
	uint64_t id;

	DataObject(uint64_t _total_size, uint64_t _id) : id(_id), total_size(_total_size), transferred_size(0)
	{
		data = (char*) malloc(total_size);
	}

	bool isReady()
	{
		return transferred_size == total_size;
	}
};

struct Thread_Footer {
	uint64_t object_id;
	uint64_t packet_id;
	uint64_t offset;
	uint64_t size;
	std::atomic<bool> ready{false};
};

struct Task {
	void* data;
	uint64_t size;
	uint64_t object_id;
	uint64_t packet_id;
	uint64_t offset;
};

class Connection
{
public:
	// virtual void send(void* data, size_t size) = 0;
	virtual void send(DataObject* object) = 0;
	virtual void* receive(DataObject* object) = 0;
	virtual void setup(bool cleanInit, size_t num_threads) = 0;
	virtual void cleanup() = 0;
	virtual ~Connection() = default;
};

class MMapConnection : public Connection
{
public:
	// void send(void* data, size_t size);
	void send(DataObject* object);
	void* receive(DataObject* object);
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

	std::queue<Task> task_queue;
	std::mutex queue_mtx;
	std::condition_variable queue_cv;

	std::atomic<uint64_t> cur_object_id;
	DataObject* cur_object;
	std::mutex obj_mtx;

	size_t local_write_seq;
	size_t local_read_seq;

	std::function<void(std::atomic<bool> *, size_t id)> thread_send;
	std::function<void(std::atomic<bool> *, size_t id)> thread_receive;

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