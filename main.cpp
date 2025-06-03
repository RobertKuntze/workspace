#include "Benchmark.hpp"
#include "connection.hpp"

#include <iostream>
#include <cstring>
#include <stddef.h>
#include <thread>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <cstring>
#include <chrono>
#include <string>
#include <iostream>

#include <iostream>
#include <getopt.h>
#include <cstdlib>

MMapConnection* con = new MMapConnection();
Benchmark bench;

int main(int argc, char* argv[]) {
    // Default values
    size_t msg_size    = 1 << 22;
	uint64_t total_size = 1ULL << 30;
    size_t num_threads  = 8;
    size_t iterations  = 100;
	size_t duration = 5;
	std::string	mode;


    // Short options: m: t: i: h for help
    const char* short_opts = "x:m:t:i:d:h:s";
    // Long options array
    const option long_opts[] = {
		{"mode",       required_argument, nullptr, 'x'},
        {"msg-size",   required_argument, nullptr, 'm'},
		{"total-size", required_argument, nullptr, 's'},
        {"threads",    required_argument, nullptr, 't'},
        {"iterations", required_argument, nullptr, 'i'},
		{"duration",   required_argument, nullptr, 'd'},
        {"help",       no_argument,       nullptr, 'h'},
        {nullptr,       0,                 nullptr,  0 }
    };

    // Parse command-line options
    while (true) {
        int opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
        if (opt == -1)
            break;

        switch (opt) {
			case 'x':
				mode = optarg;
				break;
            case 'm':
                msg_size   = 1 << std::stoul(optarg);
                break;
			case 's':
				total_size = 1 << std::stoull(optarg);
				break;
            case 't':
                num_threads = std::stoul(optarg);
                break;
            case 'i':
                iterations = std::stoul(optarg);
                break;
			case 'd':
				duration = std::stoul(optarg);
				break;
            case 'h':
            case '?':
                std::cout << "Usage: " << argv[0]
                          << " [--mode (s)ender|(r)eceiver] [--msg_size size exponent e.g. 20 => 2^20 ~ 1 MB] [--num_thread number of threads for sending AND receiving] [--iterations Iterations for latency test] [--duration amount of seconds for send benchmark]" << std::endl;
                return 0;
            default:
                break;
        }
    }

	if(mode == "sb") {
		con->setup(true, num_threads);
		bench.sendBandwidthTest(duration, msg_size);
	}

	if(mode == "sbt") {
		con->setup(true, num_threads);
		bench.sendBandwidthTotalSizeTest(msg_size, total_size);
	}

	if(mode == "rb") {
		con->setup(false, num_threads);
		bench.receiveBandwidthTest(msg_size);
	}

	if(mode == "sl") {
		con->setup(true, num_threads);
		bench.sendLatencyTest(msg_size, iterations);
	}

	if(mode == "rl") {
		con->setup(false, num_threads);
		bench.receiveLatencyTest(msg_size, iterations);
	}

	con->cleanup();
 
	return 0;
}
