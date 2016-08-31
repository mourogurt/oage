#include <iostream>
#include <experimental/tuple>
#include <threadpool.hpp>
#include <experimental/any>
#include <utils.hpp>

std::mutex pool_lock,out_lock;

int main() {
    auto event_pool = Singleton<std::vector<std::unique_ptr<EventBase>>>::getHandler();
    auto pool = Singleton<ThreadPool<>>::getHandler(std::thread::hardware_concurrency(),std::thread::hardware_concurrency());
    {
        std::lock_guard<decltype (pool_lock)> lk(pool_lock);
        event_pool->emplace_back(make_event_base_ptr([]{
            EventBase* base_event;
            auto event_pool = Singleton<std::vector<std::unique_ptr<EventBase>>>::getHandler();
            auto pool = Singleton<ThreadPool<>>::getHandler(std::thread::hardware_concurrency(),std::thread::hardware_concurrency());
            {
                std::lock_guard<decltype (pool_lock)> lk(pool_lock);
                event_pool->emplace_back(make_event_base_ptr([](const auto& item) {
                    std::lock_guard<decltype (out_lock)> lk(out_lock);
                    std::cout << item << std::endl;
                },0u));
                auto index = event_pool->size() - 1;
                base_event = event_pool->at(index).get();
            }
            TaskBase* task;
            for (auto i = 0u; i < 100u; ++i) {
                task = base_event->generateTask().release();
                set_task_values(task,i);
                pool->push(task,i % pool->queue_size());
            }
        }));
        auto index = event_pool->size() - 1;
        auto base_event = event_pool->at(index).get();
        auto task = base_event->generateTask();
        pool->push(task.release(),0);
    }
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1ms);
    pool->wait_for_empty();
}
