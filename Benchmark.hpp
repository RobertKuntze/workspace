#include <stddef.h>

class Benchmark
{
public:
	void sendBandwidthTest(size_t length_seconds, size_t msg_size);
	void receiveBandwidthTest(size_t msg_size);
	void sendLatencyTest(size_t msg_size, size_t iterations);
	void receiveLatencyTest(size_t msg_size, size_t iterations);
};