#include <stddef.h>
#include <atomic>
#include <vector>

const char HEADERPATH[] = "/header";
const char FILEPATH[] = "/file";
const unsigned long long MMAP_FILESIZE = 1ULL << 33;
const unsigned long HEADER_DAT_SIZE = 1UL << 12;

struct DataAccessEntry {
	unsigned int offset;
	unsigned int size;
};

struct Header {
	bool status;
	unsigned long index;
	DataAccessEntry DAT[HEADER_DAT_SIZE];

	Header() {
		status = 0;
		index = 0;
		DAT[0] = {DataAccessEntry{0,0}};
	}
};

class Connection
{
public:
	virtual void send(void* data, size_t size) = 0;
	virtual void send(void* data, size_t size, size_t num_threads) = 0;
	virtual void* receive(unsigned long read_index, size_t& size) = 0;
	virtual void setup() = 0;
	virtual void cleanup() = 0;
	virtual ~Connection() = default;
};

class MMapConnection : public Connection
{
public:
	void send(void* data, size_t size);
	void send(void* data, size_t size, size_t num_threads);
	void* receive(unsigned long read_index, size_t& size);
	void setup();
	void initHeader();
	void cleanup();

	Header* header;
	void* mmap_ptr;
};

class MMapConnection2 : public Connection
{
public:
	void send(void* data, size_t size);
	void* receive(unsigned long read_index, size_t& size);
	void setup();
	void cleanup();
	
	std::vector<void*> mmap_buffers;
	Header* header;
};