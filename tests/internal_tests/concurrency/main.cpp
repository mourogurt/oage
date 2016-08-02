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
#include <bitset>

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
        auto v = size - k * queues_size;
        if (queues_size % v == 0) {
            k = queues_size / v;
            for (auto i = 0u; i < v; ++i) {
                for (auto j = 0u; j < k; ++j)
                    thread_index[j+i*k].push_back(i);
                workers.emplace_back([this](unsigned num_queues, unsigned thread_id) {
                    std::mutex mut;
                    std::unique_lock<std::mutex> lk(mut);
                    std::vector<bool> all_finished (num_queues,false);
                    auto count = 0u;
                    while (true) {
                        //TODO:(High) Add barrier
                        //TODO:(Critical) add soft barrier for max queue
                        thread_cond[thread_id].wait(lk,[this,&num_queues,&thread_id]{
                            if (stoped) return true;
                            for (unsigned i = 0; i < num_queues; ++i) {
                                if (!queues[i+thread_id*num_queues].empty()) return true;
                            }
                            std::cout << "All is empty!\n";
                            return false;
                        });
                        for (unsigned i = 0; i < num_queues; ++i) {
                            if (!queues[i+thread_id*num_queues].empty()) {
                                queues[i+thread_id*num_queues].consume_one([](auto i) {
                                    (*i)();
                                    delete i;
                                });
                            } else {
                                if (stoped) {
                                    if (!all_finished[i]) { count++; all_finished[i]=!all_finished[i]; }
                                    else if (count ==num_queues ) return;
                                }
                            }
                        }
                    }
                },k,i);
            }
        } //TODO: (Critical) Add third situation
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
    ThreadPool<> pool(2,8);
    for (auto i = 0u; i < pool.queue_size(); ++i)
        pool.push(make_event_ptr_base([](auto i){
            std::cout<<i << std::endl ;
        },i).release(),i);
}
