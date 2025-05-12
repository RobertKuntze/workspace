#include <stddef.h>

class Benchmark
{
public:
	void latencyTestSend(size_t msgSize, size_t iterations);
	void LatencyTestReceive(size_t iterations);

	void sendBandwidthTest(size_t length_seconds, size_t msg_size, size_t num_threads);
};