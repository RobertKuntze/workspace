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

extern size_t PACKET_SIZE;

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

	// madvise(mmap_ptr, MMAP_FILESIZE, MADV_WILLNEED | MADV_HUGEPAGE);

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

	thread_worker = [this](std::atomic<bool> *stop_flag, size_t num_thread){
		// std::cout << "Thread " << thread << " started" << std::endl;
		while (!stop_flag->load()) {
			if (!task_queue.empty()) {
				std::unique_lock lock(queue_mtx);
				if (task_queue.empty()) { continue; }
				Task task = task_queue.front();
				task_queue.pop();
				lock.unlock();
				memcpy(task.destination, task.source, task.size);
			}
		}
		// std::unique_lock<std::mutex> lock(*cout_mut);
		// std::cout << sched_getcpu() << ",";
		// lock.unlock();
		// std::cout << "Thread " << thread << " running on  CPU " << sched_getcpu() << std::endl;
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
		send_threads.emplace_back(std::make_unique<std::thread>(thread_worker, &stop_flag, num_threads));
		receive_threads.emplace_back(std::make_unique<std::thread>(thread_worker, &stop_flag, num_threads));
		
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

void MMapConnection::completeMessage()
{
	std::unique_lock lock(queue_mtx);
	if (task_queue.empty()) return;
	Task cur = task_queue.front();
	lock.unlock();
	header->write_seq.store(cur.seq - 1);
}

void MMapConnection::completeAllMessages()
{
	while (!task_queue.empty())
	{
		std::unique_lock lock(queue_mtx);
		Task cur = task_queue.front();
		lock.unlock();
		while (cur.seq > header->write_seq.load())
		{
			header->write_seq.store(cur.seq);
			lock.lock();
			cur = task_queue.front();
			lock.unlock();
		}
	}
	if (task_queue.empty()) {
		header->write_seq.store(local_write_seq);
	}
}

void MMapConnection::send(void* data, size_t size)
{
	if (local_write_seq > header->write_seq.load()) {
		completeMessage();
	}

	while ((local_write_seq + 1) % HEADER_DAT_SIZE == header->read_seq.load() % HEADER_DAT_SIZE) {
		completeMessage();
	}
	uint64_t offset;
	
	if (local_write_seq % HEADER_DAT_SIZE == 0) {
		offset = 0;
	} else {
		offset = header->DAT[(local_write_seq - 1) % HEADER_DAT_SIZE ].offset + header->DAT[(local_write_seq - 1) % HEADER_DAT_SIZE].size;
	}

	if (offset + size > MMAP_FILESIZE) {
		// std::cerr << "Filesize exceeded" << std::endl;
		offset = 0;
	}
	
	size_t num_packets = size / PACKET_SIZE;
	size_t remainder = size % PACKET_SIZE;

	for (size_t i = 0; i < num_packets; i++) {
		std::unique_lock lock(queue_mtx);
		task_queue.push(Task {mmap_ptr + offset + (PACKET_SIZE*i), data + (PACKET_SIZE*i), PACKET_SIZE, local_write_seq});
		lock.unlock();
	}
	if (remainder != 0) {
		std::unique_lock lock(queue_mtx);
		task_queue.push(Task {mmap_ptr + offset + PACKET_SIZE*num_packets, data + PACKET_SIZE*num_packets, remainder, local_write_seq});
		lock.unlock();
	}

	
	header->DAT[local_write_seq % HEADER_DAT_SIZE] = {offset, size, false};
	local_write_seq++;
}

void* MMapConnection::receive(void* receive_buffer_)
{	
	receive_buffer = (char*) receive_buffer_;

	DataAccessEntry cur = header->DAT[header->read_seq.load() % HEADER_DAT_SIZE];

	size_t num_packets = cur.size / PACKET_SIZE;
	size_t remainder = cur.size % PACKET_SIZE;

	for (size_t i = 0; i < num_packets; i++) {
		std::unique_lock lock(queue_mtx);
		task_queue.push(Task {receive_buffer + (PACKET_SIZE*i), mmap_ptr + cur.offset + (PACKET_SIZE*i), PACKET_SIZE, header->read_seq.load()});
		lock.unlock();
	}
	std::unique_lock lock(queue_mtx);
	task_queue.push(Task {receive_buffer + PACKET_SIZE*num_packets, mmap_ptr + cur.offset + PACKET_SIZE*num_packets, remainder, header->read_seq.load()});
	lock.unlock();

	while(!task_queue.empty()) {}

	header->read_seq.fetch_add(1);
	return receive_buffer;
}
