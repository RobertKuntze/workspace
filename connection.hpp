#include <stddef.h>
#include <atomic>
#include <vector>

const char HEADERPATH[] = "/header";
const char FILEPATH[] = "/file";
const unsigned long long MMAP_FILESIZE = 1ULL << 33;
const long HEADER_DAT_SIZE = 4096;

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
	virtual long receive(unsigned long read_index, size_t& size) = 0;
	virtual void setup() = 0;
	virtual void cleanup() = 0;
	virtual ~Connection() = default;
};

class MMapConnection : public Connection
{
public:
	void send(void* data, size_t size);
	long receive(unsigned long read_index, size_t& size);
	void setup();
	void initHeader();
	void cleanup();

	Header* header;
	void* mmap_ptr;
};