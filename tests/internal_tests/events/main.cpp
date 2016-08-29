#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
#include <experimental/tuple>
#include <atomic>
#include <mutex>
#include <threadpool.hpp>


namespace EngineExperiments {

template <class... Args>
decltype (auto) make_shared_tuple(Args&& ...args) {
    return std::make_tuple(std::make_shared<typename std::decay<Args>::type...>(std::move(args))...);
}

template <class... T, std::size_t... I>
decltype(auto) unpack_ptr_tuple(const std::tuple<T...>& t, std::index_sequence<I...>) {
  return std::make_tuple(*std::get<I>(t)...);
}


template <class... Args>
decltype (auto) make_tuple_from_unique_tuple(const std::tuple<Args...>& t) {
    return unpack_ptr_tuple(t, std::make_index_sequence<sizeof...(Args)>());
}

template <class... T, std::size_t... I>
decltype(auto) make_new_tuple(const std::tuple<T...>& t, std::index_sequence<I...>) {
  return std::make_tuple(std::get<I>(t)...);
}

template <class... Args>
decltype (auto) copy_tuple(const std::tuple<Args...>& t) {
    return make_new_tuple(t, std::make_index_sequence<sizeof...(Args)>());
}

//TODO: Critical add Var Table to Task
struct TaskBase {
    TaskBase() {}
    TaskBase(const TaskBase&) {}
    TaskBase(TaskBase&&) {}
    virtual void operator ()() = 0;
    virtual ~TaskBase();
};

TaskBase::~TaskBase() {
}

template <class Func, class Tuple>
struct Task : TaskBase {
    Task() = delete;
    Task(const Func& foo, const Tuple& t) noexcept: event_func(foo), t(t) {}
    Task(Func&& foo, Tuple&& t) noexcept : event_func(std::move(foo)) , t(std::move(t)) {}
    Func event_func;
    Tuple t;
    virtual void operator() () override {
        std::experimental::apply(event_func,t);
    }

    virtual ~Task() override {
    }
};

class EventBase {
    std::mutex mut;
public:
    EventBase() {}
    EventBase(const EventBase&) {}
    EventBase(EventBase&&) {}
    decltype (auto) lock() { return mut.lock(); }
    decltype (auto)  unlock() { return mut.unlock(); }
    decltype (auto)  try_lock() { return mut.try_lock(); }
    virtual std::unique_ptr<TaskBase> generateTask () = 0;
    virtual ~EventBase();
};

EventBase::~EventBase() {
}

template <class Tuple>
class EventVarTable : public EventBase {
protected:
    Tuple t;
public:
    EventVarTable() = delete ;
    EventVarTable(const EventVarTable& other_event) noexcept : EventVarTable(other_event.t) {}
    EventVarTable(EventVarTable&& other_event) noexcept : EventVarTable(other_event.t) {}
    EventVarTable (const Tuple& t) noexcept: t(t) {}
    EventVarTable (Tuple&& t) noexcept: t(std::move(t)) {}
    virtual Tuple getVarPointersTable () {
        return EventVarTable<Tuple>::t;
    }
    virtual std::unique_ptr<TaskBase> generateTask () = 0;
    virtual ~EventVarTable();
};

template <class Tuple>
EventVarTable<Tuple>::~EventVarTable() {
}

template <class Func, class Tuple>
struct Event : EventVarTable<Tuple> {
    Func event_func;
public:
    Event() = delete ;
    Event(const Event& other_event) noexcept : Event(other_event.event_func,other_event.t) {}
    Event(Event&& other_event) noexcept : Event(other_event.event_func,other_event.t) {}
    Event(const Func& foo, const Tuple& t) noexcept: EventVarTable<Tuple>(t), event_func(foo) {}
    Event(Func&& foo, Tuple&& t) noexcept : EventVarTable<Tuple>(std::move(t)), event_func(std::move(foo)) {}
    virtual Tuple getVarPointersTable () {
        return EventVarTable<Tuple>::t;
    }
    virtual std::unique_ptr<TaskBase> generateTask () {
        auto tuple = make_tuple_from_unique_tuple(EventVarTable<Tuple>::t);
        return std::unique_ptr<TaskBase>(new Task<typename std::decay<Func>::type,decltype (tuple)>(std::move(event_func),std::move(tuple)));
    }
    virtual ~Event();
};

template <class Func, class Tuple>
Event<Func,Tuple>::~Event() {
}

template <typename T, typename Tuple>
decltype (auto) make_event_tuple (T&& t, Tuple&& tuple) {
    return Event<typename std::decay<T>::type,Tuple> (std::move(t),tuple);
}

template <typename T, typename... Args>
decltype (auto) make_event(T&& t, Args&& ...args) {
    auto unique_tuple = make_shared_tuple(std::move(args)...);
    return Event<typename std::decay<T>::type, decltype (unique_tuple)> (std::move(t),std::move(unique_tuple));
}

template <typename T, typename... Args>
decltype (auto) make_event_ptr(T&& t, Args&& ...args) {
    auto unique_tuple = make_shared_tuple(std::move(args)...);
    return std::make_unique<Event<typename std::decay<T>::type, decltype (unique_tuple)>>(std::move(t),std::move(unique_tuple));
}

template <typename T, typename... Args>
decltype (auto) make_event_base_ptr(T&& t, Args&& ...args) {
    auto unique_tuple = make_shared_tuple(std::move(args)...);
    return std::unique_ptr<EventBase>(new Event<typename std::decay<T>::type, decltype (unique_tuple)>(std::move(t),std::move(unique_tuple)));
}

}

template <class T>
void print(const T& item) {
    std::cout << item << " ";
}

template<class F, class...Args>
void for_each_arg(F&&f,Args&&...args){
  using discard=int[];
  (void)discard{0,((void)(
    f(std::forward<Args>(args))
  ),0)...};
}

template<size_t... Is, class Tuple, class F>
void for_each_tuple( std::index_sequence<Is...>, Tuple&& tup, F&& f ) {
  using std::get;
  for_each_arg(
    std::forward<F>(f),
    get<Is>(std::forward<Tuple>(tup))...
  );
}

template <class Tuple>
constexpr decltype (auto) sizeof_tuple(__attribute__((unused)) Tuple&& t) {
    return std::tuple_size<typename std::decay<Tuple>::type>::value;
}

std::vector<std::unique_ptr<EngineExperiments::EventBase>> event_pool;
std::mutex pool_lock;
ThreadPool<20000,EngineExperiments::TaskBase*> pool(std::thread::hardware_concurrency(),1);

int main() {
    {
        std::lock_guard<decltype (pool_lock)> lk(pool_lock);
        event_pool.emplace_back(EngineExperiments::make_event_base_ptr([]{
            std::lock_guard<decltype (pool_lock)> lk(pool_lock);
            event_pool.emplace_back(EngineExperiments::make_event_base_ptr([](const auto& item) {
                std::cout << item << std::endl;
            },0u));
            auto index = event_pool.size() - 1;
            std::shared_ptr<unsigned> ptr;
            auto base_event = event_pool[index].get();
            EngineExperiments::TaskBase* task;
            for (auto i = 0u; i < 100u; ++i) {
                {
                    std::lock_guard<decltype (*base_event)> ev_lk(*base_event);
                    std::tie(ptr) = dynamic_cast<EngineExperiments::EventVarTable<std::tuple<decltype (ptr)>>*>(base_event)->getVarPointersTable();
                    *ptr = i;
                    task = base_event->generateTask().release();
                }
                pool.push(task,0);
            }
        }));
        auto index = event_pool.size() - 1;
        auto base_event = event_pool[index].get();
        std::lock_guard<decltype (*base_event)> ev_lk(*base_event);
        auto task = base_event->generateTask();
        pool.push(task.release(),0);
    }
    pool.wait_for_empty();
    /*int a = 3;
    auto event = EngineExperiments::make_event_base_ptr([](const auto& item){
        std::cout << item << std::endl;
    },a);
    auto task1 = event->generateTask();
    std::shared_ptr<int> ptr;
    std::tie(ptr) = dynamic_cast<EngineExperiments::EventVarTable<std::tuple<decltype (ptr)>>*>(event.get())->getVarPointersTable();
    *ptr = 2;
    auto task2=event->generateTask();
    (*task1)();
    (*task2)();*/
}
