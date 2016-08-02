#include <iostream>
#include <thread_guard.hpp>
#include <event.hpp>
#include <deque>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <array>
#include <algorithm>
#include <boost/lockfree/queue.hpp>
#include <vector>
#include <condition_variable>
#include <ctime>

template <std::size_t Size = 20000, class Type = EventBase*, class Queue = boost::lockfree::queue<Type,boost::lockfree::capacity<Size>> ,class Container = std::vector<Queue>>
class ThreadPool {
    Container queues;
    std::vector<std::vector<unsigned>> thread_index;
    std::vector<std::condition_variable> thread_cond;
    std::vector<std::thread> workers;
    std::atomic_bool stoped { false };
    std::mutex mutex;
public:
    //TODO:(Medium) Add copy and move constructor
    explicit ThreadPool(const unsigned int& size, const unsigned int& queues_size) :queues(queues_size), thread_index(queues_size), thread_cond(size) {
        auto k = size / queues_size ;
        for (auto i = 0u, l = 0u; i < queues.size(); ++i) {
            for (auto j = 0u; j < k; ++j, ++l) {
                thread_index[i].push_back(l);
                workers.emplace_back([this](unsigned id, unsigned thread_id) {
                    std::mutex mut;
                    std::unique_lock<std::mutex> lk(mut);
                    while (true) {
                        //TODO:(High) Add barrier
                        //TODO:(Critical) add soft barrier for max queue
                        thread_cond[thread_id].wait(lk,[this,&id]{ return !queues[id].empty() || stoped; });
                        if (!queues[id].empty()) {
                            queues[id].consume_one([](auto i) {
                                (*i)();
                                delete i;
                            });
                        } else if (stoped) return;
                    }
                },i,l);
            }
        }

        //TODO:(Critical) Add second and third situation of threads
    }
    bool push(const Type& T ,const unsigned& que_num) {
        if (!stoped) {
            queues[que_num].push(T); //TODO: (High) Max queue size
            for (auto i = 0u; i < thread_index[que_num].size(); ++i) {
                thread_cond[thread_index[que_num][i]].notify_one();
            }
            return true;
        }
        return false;
    }
    bool push(Type&& T ,const unsigned& que_num) {
        if (!stoped) {
            queues[que_num].push(std::move(T)); //TODO: (High) Max queue size
            for (auto i = 0u; i < thread_index[que_num].size(); ++i) {
                thread_cond[thread_index[que_num][i]].notify_one();
            }
            return true;
        }
        return false;
    }
    //TODO: (High) buffer push

    size_t queue_size() const noexcept {
        return queues.size();
    }
    //TODO:(Medium) add clear queues
    ~ThreadPool() {
        stoped.exchange(true);
        for (auto& cond: thread_cond) {
            cond.notify_one();
        }
        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }
};

int main () {
    ThreadPool<> pool(std::thread::hardware_concurrency(),4);
    const unsigned tasks = 1000;
    std::mutex mut;
    std::unique_lock<std::mutex> lk(mut);
    std::atomic_uint counter {0};
    std::condition_variable var;
    auto start {std::chrono::steady_clock::now()}, end{start};
    for (auto i = 0u; i < tasks; i++) {
        auto ptr = make_event_ptr_base([&counter,&var](){
            srand(0);
            for (auto n = 0; n < 1; ++n) {
                std::array<int, 200000> buff;
                for (unsigned i = 0; i < 200000; ++i) {
                    buff[i] = rand();
                }
                std::sort(buff.begin(),buff.end());
            }
            counter++;
            var.notify_one();
        });
        pool.push(ptr.release(),i % pool.queue_size());
    }
    var.wait(lk,[&counter,&pool]{return counter == tasks;});
    end = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << std::endl;
    counter = 0;
    start = std::chrono::steady_clock::now();
    for (auto i = 0u; i < tasks; i++) {
        std::thread(make_event([&counter,&var](){
            srand(0);
            for (auto n = 0; n < 1; ++n) {
                std::array<int, 200000> buff;
                for (unsigned i = 0; i < 200000; ++i) {
                    buff[i] = rand();
                }
                std::sort(buff.begin(),buff.end());
            }
            counter++;
            var.notify_one();
        })).detach();
    }
    var.wait(lk,[&counter,&pool]{return counter == tasks;});
    end = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << std::endl;

}
