#include <thread>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <condition_variable>

std::vector<std::thread> threads;
std::vector<std::unique_ptr<std::condition_variable>> cv_;
std::vector<std::mutex*> mtx;

void worker(int thread_id, int num_threads) {
    std::cout << "Thread " << thread_id << " started" << std::endl;
    while (true) {
        std::unique_lock<std::mutex> lock(*mtx[thread_id]);
        // Perform work here
        std::cout << "Thread " << thread_id << " is working" << std::endl;
        cv_[thread_id]->wait(lock);
    }
}

int main(int argc, char const *argv[])
{
    for (size_t i = 0; i < 4; i++) {
        mtx.push_back(new std::mutex());
        cv_.emplace_back(std::make_unique<std::condition_variable>());
    }

    for (size_t i = 0; i < 4; i++) {
        threads.emplace_back(worker, i, 4);
    }

    for (size_t i = 0; i < 4; i++) {
        sleep(1);
        cv_[i]->notify_one();
    }

    for (auto& t : threads) {
        t.join();
    }
    for (auto& m : mtx) {
        delete m;
    }
    mtx.clear();
    cv_.clear();
    threads.clear();

    return 0;
}
