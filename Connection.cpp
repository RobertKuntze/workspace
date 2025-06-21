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
	if (ftruncate(fd, MMAP_SIZE_PER_THREAD * num_threads)) {
		std::cerr << "ftruncate Error Data" << std::endl;
	}

	header = (Header*) mmap(nullptr, sizeof(Header), PROT_READ | PROT_WRITE, MAP_SHARED, fd_h, 0);

	mmap_ptr = mmap(nullptr, MMAP_SIZE_PER_THREAD * num_threads, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

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

		for (size_t i = 1; i <= num_threads; i++) {
			Thread_Footer* footer = (Thread_Footer*) (mmap_ptr + (MMAP_SIZE_PER_THREAD * i)) - 1;
			new (footer) Thread_Footer;
		}

		// int status;
		// int ret = move_pages(0, 1, &mmap_ptr, nullptr, &status, 0);
		// if (ret == -1) {
		// 	perror("move_pages");
		// }

		// std::cout << "Memory is located on NUMA node: " << status << std::endl;
	}
	
	close(fd);
	close(fd_h);

	thread_send = [this](std::atomic<bool> *stop_flag, size_t id){
		// std::cout << "Thread " << thread << " started" << std::endl;
		void* target_data = mmap_ptr + (MMAP_SIZE_PER_THREAD * id);
		Thread_Footer* footer = (Thread_Footer*) (mmap_ptr + (MMAP_SIZE_PER_THREAD * (id+1)) - sizeof(Thread_Footer));
		// new (footer) Thread_Footer;
		while (!stop_flag->load()) {
			std::unique_lock lock(queue_mtx);
			queue_cv.wait(lock, [&]() { return !footer->ready.load() || stop_flag->load(); });
			if (task_queue.empty()) { continue; }
			Task task = task_queue.front();
			task_queue.pop();
			lock.unlock();
			if (task.size > MMAP_SIZE_PER_THREAD - sizeof(Thread_Footer)) {
				continue;
			}
			while (footer->ready.load()) {}
			memcpy(target_data, task.data, task.size);
			footer->object_id = task.object_id;
			footer->packet_id = task.packet_id;
			footer->offset = task.offset;
			footer->size = task.size;
			footer->ready.store(true);
		}
		// std::unique_lock<std::mutex> lock(*cout_mut);
		// std::cout << sched_getcpu() << ",";
		// lock.unlock();
		// std::cout << "Thread " << thread << " running on  CPU " << sched_getcpu() << std::endl;
	};

	thread_receive = [this](std::atomic<bool> *stop_flag, size_t id){
		void* source_data = mmap_ptr + (MMAP_SIZE_PER_THREAD * id);
		Thread_Footer* footer = (Thread_Footer*) (mmap_ptr + (MMAP_SIZE_PER_THREAD * (id+1)) - sizeof(Thread_Footer));
		while(!stop_flag->load()) {
			while(!object_ready.load()) {
				if (stop_flag->load()) {
					return;
				}
			}
			// wait for footer ready
			while(!footer->ready.load()) {
				if (stop_flag->load()) {
					return;
				}
			}

			// skip if object is not current target
			if (cur_object_id != footer->object_id) {
				footer->ready.store(false);
				continue;
			}

			memcpy(cur_object->data + footer->offset, source_data, footer->size);
			std::unique_lock lock(obj_mtx);
			cur_object->transferred_size += footer->size;
			footer->ready.store(false);
		}
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
		send_threads.emplace_back(std::make_unique<std::thread>(thread_send, &stop_flag, i));
		receive_threads.emplace_back(std::make_unique<std::thread>(thread_receive, &stop_flag, i));
		
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
	queue_cv.notify_all();
	for (size_t i = 0; i < num_threads; i++) {
		if (send_threads[i]->joinable()) {
			send_threads[i]->join();
		}
		if (receive_threads[i]->joinable()) {
			receive_threads[i]->join();
		}
	}
	// std::cout << "]," << std::endl;
	munmap(header, sizeof(Header));
	munmap(mmap_ptr, MMAP_SIZE_PER_THREAD * num_threads);
}

void MMapConnection::send(DataObject* obj)
{
	uint64_t num_packets = obj->total_size / PACKET_SIZE;
	uint64_t remainder = obj->total_size % PACKET_SIZE;
	if (remainder != 0) { num_packets++; }

	for (uint64_t i = 0; i < num_packets; i++) {
		uint64_t offset = PACKET_SIZE * i;
		std::unique_lock lock(queue_mtx);
		task_queue.push(Task{obj->data + offset, (remainder == 0 ? PACKET_SIZE : remainder), obj->id, i, offset});
		queue_cv.notify_one();
		lock.unlock();
	}

	while(1)
	{
		std::unique_lock lock(queue_mtx);
		if (task_queue.empty()) {
			lock.unlock();
			break;
		}
		lock.unlock();
		queue_cv.notify_all();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void MMapConnection::receive(DataObject* object)
{
	std::unique_lock lock(obj_mtx);
	cur_object = object;
	lock.unlock();
	cur_object_id = object->id;
	object_ready.store(true);
}
