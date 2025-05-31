#include "connection.hpp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <sched.h>
#include <numa.h>
#include <numaif.h>


void MMapConnection::setup(bool cleanInit = false, size_t num_threads_ = 1)
{
	num_threads = num_threads_;
	fd_h = shm_open(HEADERPATH, O_CREAT | O_RDWR, 0666);
	if (fd_h == -1) {
		std::cerr << "shm_open Error Header" << std::endl;
	}

	if (cleanInit) { ftruncate(fd_h, 0); }
	if (ftruncate(fd_h, sizeof(Header))) {
		std::cerr << "ftruncate Error Header" << std::endl;
	}

	fd = shm_open(FILEPATH, O_CREAT | O_RDWR, 0666);
	if (fd == -1) {
		std::cerr << "shm_open Error Data" << std::endl;
	}

	if (cleanInit) { ftruncate(fd, 0); }
	if (ftruncate(fd, MMAP_FILESIZE)) {
		std::cerr << "ftruncate Error Data" << std::endl;
	}

	header = (Header*) mmap(nullptr, sizeof(Header), PROT_READ | PROT_WRITE, MAP_SHARED, fd_h, 0);

	mmap_ptr = mmap(nullptr, MMAP_FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	// sleep(1);

	if (header == MAP_FAILED) {
		std::cerr << "mmap Error Header" << std::endl;
	}
	if (mmap_ptr == MAP_FAILED) {
		std::cerr << "mmap Error Data" << std::endl;
	}

	// if (cleanInit) {
	// 	numa_tonode_memory(header, sizeof(Header), 0);
	// 	numa_tonode_memory(mmap_ptr, MMAP_FILESIZE, 0);
	// }

	if (cleanInit) {
		initHeader();

		// int status;
		// int ret = move_pages(0, 1, &mmap_ptr, nullptr, &status, 0);
		// if (ret == -1) {
		// 	perror("move_pages");
		// }

		// std::cout << "Memory is located on NUMA node: " << status << std::endl;
	}
	
	close(fd);
	close(fd_h);

	thread_send = [this](std::atomic<bool> *stop_flag, size_t thread, size_t num_thread){
		// std::cout << "Thread " << thread << " started" << std::endl;
		while (!stop_flag->load()) {
			std::unique_lock<std::mutex> lock(*mtx_s[thread]);
			cv_s[thread]->wait(lock, [&] {
				return data_ready[thread]->load() || stop_flag->load();
			});
			if (data_ready[thread]->load()) {
				memcpy(mmap_ptr + target_offsets[thread], data_ptrs[thread], data_sizes[thread]);
				data_ready[thread]->store(false);
			}
		}
		// std::unique_lock<std::mutex> lock(*cout_mut);
		// std::cout << sched_getcpu() << ",";
		// lock.unlock();
		// std::cout << "Thread " << thread << " running on  CPU " << sched_getcpu() << std::endl;
	};

	thread_receive = [this](std::atomic<bool> *stop_flag, size_t thread, size_t num_thread){
		while (!stop_flag->load()) {
			std::unique_lock<std::mutex> lock(*mtx_r[thread]);
			cv_r[thread]->wait(lock, [&]{
				return receive_ready[thread]->load() || stop_flag->load();
			});
			if (receive_ready[thread]->load()) {
				memcpy(receive_buffer + receive_offsets[thread], mmap_ptr + receive_ptrs[thread], receive_sizes[thread]);
				receive_ready[thread]->store(false);
			}
		}
		// std::unique_lock<std::mutex> lock(*cout_mut);
		// std::cout << sched_getcpu() << ",";
		// lock.unlock();
	};

	// data_ptrs.resize(num_threads);
	// data_sizes.resize(num_threads);
	// target_offsets.resize(num_threads);
	// receive_ptrs.resize(num_threads);
	// receive_sizes.resize(num_threads);
	// receive_offsets.resize(num_threads);
	cv_s.reserve(num_threads);
	cv_r.reserve(num_threads);
	// send_threads.reserve(num_threads);
	// mtx.reserve(num_threads);

	cpu_set_t cpuset;

	for (size_t i = 0; i < num_threads; i++)
	{
		mtx_s.emplace_back(std::make_unique<std::mutex>());
		mtx_r.emplace_back(std::make_unique<std::mutex>());
		cv_s.emplace_back(std::make_unique<std::condition_variable>());
		cv_r.emplace_back(std::make_unique<std::condition_variable>());
		data_ready.emplace_back(std::make_unique<std::atomic<bool>>(false));
		receive_ready.emplace_back(std::make_unique<std::atomic<bool>>(false));
		data_ptrs.emplace_back(nullptr);
		data_sizes.emplace_back(0);
		receive_ptrs.emplace_back(0);
		receive_sizes.emplace_back(0);
		receive_offsets.emplace_back(0);
		target_offsets.emplace_back(0);
	}

	size_t cpu_binds[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	
	for (size_t i = 0; i < num_threads; i++) {
		send_threads.emplace_back(std::make_unique<std::thread>(thread_send, &stop_flag, i, num_threads));
		receive_threads.emplace_back(std::make_unique<std::thread>(thread_receive, &stop_flag, i, num_threads));
		
		CPU_ZERO(&cpuset);
		CPU_SET(cpu_binds[i], &cpuset);
		if (pthread_setaffinity_np(send_threads.back()->native_handle(), sizeof(cpu_set_t), &cpuset) != 0) { std::cout << "pthread set affinity failed" << std::endl; }
		
		CPU_ZERO(&cpuset);
		CPU_SET(cpu_binds[i+8], &cpuset);
		if (pthread_setaffinity_np(receive_threads.back()->native_handle(), sizeof(cpu_set_t), &cpuset) != 0) { std::cout << "pthread set affinity failed" << std::endl; }
	}
}

void MMapConnection::initHeader()
{
	new (header) Header{};
}

void MMapConnection::cleanup()
{
	stop_flag = true;
	// std::cout << "\"send_cpus\" : [";
	for (size_t i = 0; i < cv_s.size(); i++) {
		cv_s[i]->notify_all();
		send_threads[i]->join();
	}
	// std::cout << "]," << std::endl;
	// std::cout << "\"receive_cpus\" : [";
	for (size_t i = 0; i < cv_r.size(); i++) {
		cv_r[i]->notify_all();
		receive_threads[i]->join();
	}
	// std::cout << "]," << std::endl;
	munmap(header, sizeof(Header));
	munmap(mmap_ptr, MMAP_FILESIZE);
}

void MMapConnection::send(void* data, size_t size, size_t num_threads = 1)
{
	// header->status = true;
	// if (header->index >= HEADER_DAT_SIZE) {
	// 	header->index=0;
	// }
	
	// DataAccessEntry cur = header->DAT[header->index];
	// uint64_t offset = cur.offset + cur.size;

	// if (offset + size > MMAP_FILESIZE) {
	// 	// std::cout << "Filesize exceeded" << std::endl;
	// 	header->index = 0;
	// 	cur = header->DAT[header->index];
	// 	offset = cur.offset + cur.size;
	// 	return;
	// }

	// std::vector<std::thread> threads;
	// size_t block_size = size / num_threads;
	// size_t remainder = size % num_threads;

	// auto thread_send = [](void* dest, void* data, size_t size) {
	// 	memcpy(dest, data, size);
	// };

	// for (size_t i = 0; i < num_threads; i++) {
	// 	size_t off = i * block_size;
	// 	size_t cur_size = i == num_threads - 1 ? block_size + remainder : block_size;

	// 	send_threads.emplace_back(thread_send, mmap_ptr + offset + off, data + off, cur_size);
	// }

	// for (auto& t : threads) {
	// 	t.join();
	// }
	
	// header->index++;
	// header->DAT[header->index] = DataAccessEntry{offset, size};
}

void MMapConnection::send(void* data, size_t size)
{
	if ((header->write_seq.load() + 1) % HEADER_DAT_SIZE == header->read_seq.load() % HEADER_DAT_SIZE) {
		// std::cout << "writer cought up with reader" << std::endl;
	}
	while ((header->write_seq.load() + 1) % HEADER_DAT_SIZE == header->read_seq.load() % HEADER_DAT_SIZE) {}
	uint64_t offset;
	
	if (header->write_seq.load() % HEADER_DAT_SIZE == 0) {
		offset = 0;
	} else {
		offset = header->DAT[(header->write_seq.load() - 1) % HEADER_DAT_SIZE].offset + header->DAT[(header->write_seq.load() - 1) % HEADER_DAT_SIZE].size;
	}

	if (offset + size > MMAP_FILESIZE) {
		// std::cerr << "Filesize exceeded" << std::endl;
		offset = 0;
	}

	size_t thread_size = size / num_threads;
	size_t remainder = size % num_threads;

	for (size_t i = 0; i < num_threads; i++) {
		if (i == 0) {
			data_ptrs[i] = data + (i * (thread_size + remainder));
			data_sizes[i] = thread_size + remainder;
			target_offsets[i] = offset + i * (thread_size + remainder);
		} else {
			data_ptrs[i] = data + (i * thread_size);
			data_sizes[i] = thread_size;
			target_offsets[i] = offset + i * (thread_size);
		}
		std::unique_lock<std::mutex> lock(*mtx_s[i]);
		data_ready[i]->store(true);
		cv_s[i]->notify_one();
		lock.unlock();
	}

	while (true) {
		bool all_ready = true;
		for (size_t i = 0; i < num_threads; i++) {
			if (data_ready[i]->load()) {
				all_ready = false;
				break;
			}
		}
		if (all_ready) {
			// std::cout << "All threads are ready" << std::endl;
			break;
		}
	}

	header->DAT[header->write_seq.load() % HEADER_DAT_SIZE] = DataAccessEntry{offset, size};
	header->write_seq.fetch_add(1);
	// std::cout << "Header seq: " << header->seq << std::endl;
}

void* MMapConnection::receive(void* receive_buffer_)
{	
	receive_buffer = (char*) receive_buffer_;

	DataAccessEntry cur = header->DAT[header->read_seq.load() % HEADER_DAT_SIZE];

	for (size_t i = 0; i < num_threads; i++) {
		receive_ptrs[i] = cur.offset + (i * cur.size / num_threads);
		receive_sizes[i] = cur.size / num_threads;
		receive_offsets[i] = (i * cur.size / num_threads);
		std::unique_lock<std::mutex> lock(*mtx_r[i]);
		receive_ready[i]->store(true);
		cv_r[i]->notify_one();
		lock.unlock();
	}

	while (true) {
		bool all_ready = true;
		for (size_t i = 0; i < num_threads; i++) {
			if (receive_ready[i]->load()) {
				all_ready = false;
				break;
			}
		}
		if (all_ready) {
			// std::cout << "All threads are ready" << std::endl;
			break;
		}
	}

	header->read_seq.fetch_add(1);
	return receive_buffer;
}
