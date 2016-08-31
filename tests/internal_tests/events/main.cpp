#include <iostream>
#include <experimental/tuple>
#include <threadpool.hpp>
#include <experimental/any>
#include <utils.hpp>

std::mutex pool_lock,out_lock;

int main() {
    auto event_pool = Singleton<std::vector<std::unique_ptr<EventBase>>>::createHandler();
    auto pool = Singleton<ThreadPool<>>::createHandler(std::thread::hardware_concurrency(),std::thread::hardware_concurrency());
    {
        std::lock_guard<decltype (pool_lock)> lk(pool_lock);
        event_pool->emplace_back(make_event_base_ptr([]{
            size_t index {0};
            auto event_pool = Singleton<std::vector<std::unique_ptr<EventBase>>>::getHandler();
            auto pool = Singleton<ThreadPool<>>::getHandler();
            {
                std::lock_guard<decltype (pool_lock)> lk(pool_lock);
                event_pool->emplace_back(make_event_base_ptr([](const auto& item) {
                    std::lock_guard<decltype (out_lock)> lk(out_lock);
                    std::cout << item << std::endl;
                },0u));
                index = event_pool->size() - 1;
            }
            for (auto i = 0u; i < 10000000u; ++i)
                pool->push(set_task_values(event_pool->at(index)->generateTask().release(),i),i % pool->queue_size());
        }));
        pool->push(event_pool->at(event_pool->size() - 1)->generateTask().release(),0);
    }
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1ms);
    pool->wait_for_empty();
}
