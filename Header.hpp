const long DATA_FILE_SIZE = 1UL << 30;
const long BLOCK_SIZE = 1UL << 20;

const long HEADER_DAT_SIZE = 4096;

struct DataAccessEntry {
	unsigned long offset;
	unsigned long size;
};

struct Header {
	unsigned char status;
	unsigned long index = 0;
	DataAccessEntry DAT[HEADER_DAT_SIZE];

	Header() {
		status = 0;
		DAT[0] = DataAccessEntry{0,0};
	}

	// returns offset of the next free slot
	void sendData(void* destination_file, void* source, unsigned long size) {
		if (index >= HEADER_DAT_SIZE) {
			return;
		}
		unsigned long offset = DAT[index].offset + DAT[index].size;

		memcpy(destination_file + offset, source, size);

		DAT[++index] = DataAccessEntry{offset, size};
	}
};