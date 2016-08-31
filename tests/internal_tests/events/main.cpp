#include <threadpool.hpp>
#include <utils.hpp>

#include <iostream>

std::mutex pool_lock,out_lock;

int main() {
    //Create singleton vector of events(event pool) and threadpool
    auto event_pool = Singleton<std::vector<std::unique_ptr<EventBase>>>::createHandler();
    //First argument is number of threads second is number of queues
    auto pool = Singleton<ThreadPool<>>::createHandler(std::thread::hardware_concurrency(),std::thread::hardware_concurrency());
    {
        //Lock pool_lock to prevent races
        std::lock_guard<decltype (pool_lock)> lk(pool_lock);
        //Creating new event
        event_pool->emplace_back(make_event_base_ptr([]{
            size_t index {0};
            //Getting pointers to event pool and threadpool
            auto event_pool = Singleton<std::vector<std::unique_ptr<EventBase>>>::getHandler();
            auto pool = Singleton<ThreadPool<>>::getHandler();
            {
                std::lock_guard<decltype (pool_lock)> lk(pool_lock);
                //Creating new event in event
                event_pool->emplace_back(make_event_base_ptr([](const auto& item) {
                    std::lock_guard<decltype (out_lock)> lk(out_lock);
                    std::cout << item << std::endl;
                },0u));
                //Getting index of sub event
                index = event_pool->size() - 1;
            }
            for (auto i = 0u; i < 10000000u; ++i)
                //Firstly we generate task using event_pool->at(index)->generateTask()
                //Then we change variables in task by set_task_values(task,i) (Note: function will return same task, it's not new task with changed vars)
                //After that we pushing it in threadpool pool->push(task, number of queue)
                pool->push(set_task_values(event_pool->at(index)->generateTask().release(),i),i % pool->queue_size());
        }));
        //Pushing task
        pool->push(event_pool->at(event_pool->size() - 1)->generateTask().release(),0);
    }
    //Weird workaround, instantly after pushing, pool will think that there is no tasks in queue(don't know why yet)
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1ms);
    //Waiting for all queues get finished their jobs
    pool->wait_for_empty();
}
