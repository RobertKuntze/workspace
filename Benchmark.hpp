#include <stddef.h>

class Benchmark
{
public:
	void latencyTestSend(size_t msgSize, size_t iterations);
	void LatencyTestReceive(size_t iterations);
};