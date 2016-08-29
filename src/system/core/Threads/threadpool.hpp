#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP
#include <event.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <vector>
#include <condition_variable>
#include <math.hpp>

//TODO: (Critical) Add event call when all queues empty 3
//TODO: (Critical) Add constructor for manual distribution threads of queues 2
//TODO: (Critical) Don't create queue in class, instead just add references 1
//TODO: (Critical) Remove notification from push and add explicit notify function
//TODO: (High) Max queue size(try-push,wait-push)
//TODO: (High) buffer push
//TODO: (Medium) Support deleting events from pool
//TODO: (Medium) Add copy and move constructor
//TODO: (Medium) add clear queues
//TODO: (Medium) Add barrier
//TODO: (Low) Resizing,Adding multipile queues if max queue size

template <std::size_t Size = 20000, class Type = EventBase*, class Queue = boost::lockfree::queue<Type,boost::lockfree::capacity<Size>> ,class Container = std::vector<Queue>>
class ThreadPool {
    Container queues;
    std::vector<std::vector<unsigned>> thread_index;
    std::vector<std::condition_variable> thread_cond;
    std::vector<std::thread> workers;
    std::atomic_bool stoped { false };
    std::mutex mutex;
    std::atomic_uint th_stoped_counter{0};
    mutable std::condition_variable stoped_cond;
public:
    explicit ThreadPool(const unsigned int& size, const unsigned int& queues_size) :queues(queues_size), thread_index(queues_size), thread_cond(size) {
        auto th_group_size = size / queues_size ;
        auto thread_offset = 0u;
        for (auto i = 0u; i < queues_size; ++i) {
            for (auto j = 0u; j < th_group_size; ++j, ++thread_offset) {
                thread_index[i].push_back(thread_offset);
                workers.emplace_back([this](const unsigned &queue_id, const unsigned &thread_id) {
                    std::mutex mut;
                    std::unique_lock<std::mutex> lk(mut);
                    while (true) {
                        if (!queues[queue_id].empty()) {
                            queues[queue_id].consume_one([](auto&& i) {
                                (*i)();
                                delete i;
                            });
                        }
                        else if (!stoped) {
                            //TODO: (High) add pause and check if queue paused or stoped
                            th_stoped_counter.fetch_add(1, std::memory_order_relaxed);
                            stoped_cond.notify_all();
                            thread_cond[thread_id].wait(lk);
                            th_stoped_counter.fetch_add(-1u, std::memory_order_relaxed);
                        }
                             else return;
                    }
                },i,thread_offset);
            }
        }
        auto threads = size - th_group_size * queues_size;
        auto group_size = gcd(threads,queues_size);
        threads /= group_size;
        auto queue_group_size = queues_size / group_size;
        for (auto i = 0u; i < group_size; ++i) {
            for (auto j = 0u; j < threads; ++j) {
                for (auto k = 0u; k < queue_group_size; ++k)
                    thread_index[k + i *queue_group_size].push_back(j + thread_offset + i * threads);
                workers.emplace_back([this](const unsigned& queue_group_size, const unsigned& group_id, const unsigned& thread_id) {
                    std::mutex mut;
                    std::unique_lock<std::mutex> lk(mut);
                    std::vector<bool> all_finished (queue_group_size,false);
                    auto count = 0u;
                    bool continue_thread{true};
                    while (true) {
                        continue_thread = stoped;
                        for (unsigned i = 0; i < queue_group_size; ++i) {
                            if (!queues[i+group_id*queue_group_size].empty()) {
                                continue_thread = true;
                                queues[i+group_id*queue_group_size].consume_one([](auto&& i) {
                                    (*i)();
                                    delete i;
                                });
                            } else {
                                if (stoped) {
                                    if (!all_finished[i]) { count++; all_finished[i]=!all_finished[i]; }
                                    else if (count ==queue_group_size ) return;
                                }
                            }
                        }
                        if (!continue_thread) {
                            //TODO: (High) add pause and check if queue paused or stoped
                            th_stoped_counter.fetch_add(1, std::memory_order_relaxed);
                            stoped_cond.notify_all();
                            thread_cond[thread_id].wait(lk);
                            th_stoped_counter.fetch_add(-1u, std::memory_order_relaxed);
                        }
                    }
                },queue_group_size,i,j + thread_offset);
            }
        }
    }

    void remove_thread();

    bool push(const Type& T ,const unsigned& que_num) {
        if (!stoped) {
            queues[que_num].push(T);
            for (auto i = 0u; i < thread_index[que_num].size(); ++i) {
                thread_cond[thread_index[que_num][i]].notify_one();
            }
            return true;
        }
        return false;
    }

    bool push(Type&& T ,const unsigned& que_num) {
        if (!stoped) {
            queues[que_num].push(std::move(T));
            for (auto i = 0u; i < thread_index[que_num].size(); ++i) {
                thread_cond[thread_index[que_num][i]].notify_one();
            }
            return true;
        }
        return false;
    }

    size_t queue_size() const noexcept {
        return queues.size();
    }




    bool is_empty() const noexcept {
        return th_stoped_counter.load() == workers.size(); //TODO: (Low) think about memory_order
    }

    void wait_for_empty() const noexcept {
        std::mutex mut;
        std::unique_lock<std::mutex> lk(mut);
        stoped_cond.wait(lk,[this] { return is_empty(); });

    }

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

#endif // THREADPOOL_HPP
