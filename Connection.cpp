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
	
	close(fd);
	close(fd_h);

	thread_send = [this](std::atomic<bool> *stop_flag, size_t thread, size_t num_thread){
		// std::cout << "Thread " << thread << " started" << std::endl;
		while (!stop_flag->load()) {
			std::unique_lock<std::mutex> lock(*mtx[2*thread]);
			cv_s[thread]->wait(lock, [&] {
				return data_ready[thread]->load() || stop_flag->load();
			});
			if (data_ready[thread]->load()) {
				// std::cout << "Offsets: " << target_offsets[thread] << std::endl;
				// std::cout << "Sizes: " << data_sizes[thread] << std::endl;
				// std::cout << "Data: " << data_ptrs[thread] << std::endl;
				memcpy(mmap_ptr + target_offsets[thread], data_ptrs[thread], data_sizes[thread]);
				// std::cout << "Thread " << thread << " finished" << std::endl;
				data_ready[thread]->store(false);
			}
			
		}
		// std::cout << "Thread " << thread << " running on  CPU " << sched_getcpu() << std::endl;
	};

	thread_receive = [this](std::atomic<bool> *stop_flag, size_t thread, size_t num_thread){
		while (!stop_flag->load()) {
			std::unique_lock<std::mutex> lock(*mtx[2*thread + 1]);
			cv_r[thread]->wait(lock, [&]{
				return receive_ready[thread]->load() || stop_flag->load();
			});
			if (receive_ready[thread]->load()) {
				memcpy(receive_buffer + receive_offsets[thread], mmap_ptr + receive_ptrs[thread], receive_sizes[thread]);
				receive_ready[thread]->store(false);
			}
		}
	};

	data_ptrs.resize(num_threads);
	data_sizes.resize(num_threads);
	target_offsets.resize(num_threads);
	// send_threads.reserve(num_threads);
	// mtx.reserve(num_threads);
	stop_flag = false;

	cpu_set_t cpuset;

	for (size_t i = 0; i < num_threads; i++)
	{
		mtx.emplace_back(std::make_unique<std::mutex>());
		mtx.emplace_back(std::make_unique<std::mutex>());
		cv_s.emplace_back(std::make_unique<std::condition_variable>());
		cv_r.emplace_back(std::make_unique<std::condition_variable>());
		data_ready.emplace_back(std::make_unique<std::atomic<bool>>(false));
		receive_ready.emplace_back(std::make_unique<std::atomic<bool>>(false));
		target_offsets[i] = 0;
		data_ptrs[i] = nullptr;
		data_sizes[i] = 0;
		send_threads.emplace_back(std::make_unique<std::thread>(thread_send, &stop_flag, i, num_threads));
		// receive_threads.emplace_back(std::make_unique<std::thread>(thread_receive, &stop_flag, i, num_threads));
		CPU_ZERO(&cpuset);
		CPU_SET(i+64, &cpuset);
		if (pthread_setaffinity_np(send_threads.back()->native_handle(), sizeof(cpu_set_t), &cpuset) != 0) { std::cout << "pthread set affinity failed" << std::endl; }
		// CPU_ZERO(&cpuset);
		// CPU_SET(i + 16, &cpuset);
		// if (pthread_setaffinity_np(receive_threads.back()->native_handle(), sizeof(cpu_set_t), &cpuset) != 0) { std::cout << "pthread set affinity failed" << std::endl; }
	}
}

void MMapConnection::initHeader()
{
	new (header) Header();
}

void MMapConnection::cleanup()
{
	stop_flag = true;
	for (size_t i = 0; i < cv_s.size(); i++) {
		cv_s[i]->notify_all();
		cv_r[i]->notify_all();
	}
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
	if (size % send_threads.size() != 0) {
		std::cerr << "Data size is not divisible by number of threads" << std::endl;
		return;
	}

	uint64_t offset = header->DAT[header->index].offset + header->DAT[header->index].size;

	if (offset + size > MMAP_FILESIZE) {
		// std::cerr << "Filesize exceeded" << std::endl;
		header->index = 0;
		offset = 0;
	}

	for (size_t i = 0; i < num_threads; i++) {
		data_ptrs[i] = data + (i * size / num_threads);
		data_sizes[i] = size / num_threads;
		target_offsets[i] = offset + i * (size / num_threads);
		std::unique_lock<std::mutex> lock(*mtx[i]);
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

	header->index++;
	header->DAT[header->index] = DataAccessEntry{offset, size};
	if (header->index >= HEADER_DAT_SIZE) {
		header->index = 0;
	}
}

void* MMapConnection::receive(size_t read_index, size_t& size)
{	
	DataAccessEntry cur = header->DAT[read_index];

	receive_buffer = new char[cur.size];

	for (size_t i = 0; i < num_threads; i++) {
		receive_ptrs[i] = cur.offset + (i * size / num_threads);
		receive_sizes[i] = cur.size / num_threads;
		receive_offsets[i] = (i * size / num_threads);
		std::unique_lock<std::mutex> lock(*mtx[i+num_threads]);
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

	return &receive_buffer;
}
