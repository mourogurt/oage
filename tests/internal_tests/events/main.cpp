#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
#include <experimental/tuple>
#include <atomic>
#include <mutex>
#include <threadpool.hpp>
#include <experimental/any>


namespace EngineExperiments {

template <class... Args>
decltype (auto) make_shared_tuple(Args&& ...args) {
    return std::make_tuple(std::make_shared<typename std::decay<Args>::type...>(std::move(args))...);
}

//TODO: Critical add Var Table to Task
class TaskBase {
public:
    TaskBase() {}
    TaskBase(const TaskBase&) {}
    TaskBase(TaskBase&&) {}
    virtual void operator ()() = 0;
    virtual ~TaskBase();
};

TaskBase::~TaskBase() {
}

template<class... Args>
struct TaskVarTable : public TaskBase {
    std::tuple<Args...> t;
public:
    TaskVarTable() = delete ;
    TaskVarTable(const TaskVarTable& other_event) noexcept : TaskVarTable(other_event.t) {}
    TaskVarTable(TaskVarTable&& other_event) noexcept : TaskVarTable(other_event.t) {}
    TaskVarTable (const std::tuple<Args...>& t) noexcept: t(t) {}
    TaskVarTable (std::tuple<Args...>&& t) noexcept: t(std::move(t)) {}
    virtual std::tuple<Args...>& getVarPointersTable () {
        return t;
    }
    virtual void operator ()() = 0;
};

template <class Func, class... Args>
class Task : public TaskVarTable<Args...> {
    Func event_func;
public:
    Task() = delete ;
    Task(const Task& other_event) noexcept : Task(other_event.event_func,other_event.t) {}
    Task(Task&& other_event) noexcept : Task(other_event.event_func,other_event.t) {}
    Task(const Func& foo, const std::tuple<Args...>& t) noexcept: TaskVarTable<Args...>(t), event_func(foo) {}
    Task(Func&& foo, std::tuple<Args...>&& t) noexcept : TaskVarTable<Args...>(std::move(t)), event_func(std::move(foo)) {}
    virtual std::tuple<Args...>& getVarPointersTable () override {
        return TaskVarTable<Args...>::t;
    }
    virtual void operator() () override {
       std::experimental::apply(event_func,TaskVarTable<Args...>::t);
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

template <class... Args>
class EventVarTable : public EventBase {
protected:
    std::tuple<Args...> t;
public:
    EventVarTable() = delete ;
    EventVarTable(const EventVarTable& other_event) noexcept : EventVarTable(other_event.t) {}
    EventVarTable(EventVarTable&& other_event) noexcept : EventVarTable(other_event.t) {}
    EventVarTable (const std::tuple<Args...>& t) noexcept: t(t) {}
    EventVarTable (std::tuple<Args...>&& t) noexcept: t(std::move(t)) {}
    virtual std::tuple<Args...> getVarPointersTable () {
        return t;
    }
    virtual std::unique_ptr<TaskBase> generateTask () = 0;
    virtual ~EventVarTable();
};

template <class... Args>
EventVarTable<Args...>::~EventVarTable() {
}

template <class Func, class... Args>
class Event : public EventVarTable<Args...> {
    Func event_func;
    template <class... T, std::size_t... I>
    decltype(auto) unpack_event(const std::tuple<T...>& t, std::index_sequence<I...>) {
      return std::unique_ptr<TaskBase>(new Task<typename std::decay<Func>::type,typename std::decay<decltype (*std::get<I>(t))>::type...>
                                       (std::move(event_func),std::make_tuple(*std::get<I>(t)...)));
    }
    template <class... Args_in>
    decltype (auto) make_task(const std::tuple<Args_in...>& t) {
        return unpack_event(t, std::make_index_sequence<sizeof...(Args_in)>());
    }
public:
    Event() = delete ;
    Event(const Event& other_event) noexcept : Event(other_event.event_func,other_event.t) {}
    Event(Event&& other_event) noexcept : Event(other_event.event_func,other_event.t) {}
    Event(const Func& foo, const std::tuple<Args...>& t) noexcept: EventVarTable<Args...>(t), event_func(foo) {}
    Event(Func&& foo, std::tuple<Args...>&& t) noexcept : EventVarTable<Args...>(std::move(t)), event_func(std::move(foo)) {}
    virtual std::tuple<Args...> getVarPointersTable () override {
        return EventVarTable<Args...>::t;
    }
    virtual std::unique_ptr<TaskBase> generateTask () override {
        return make_task(EventVarTable<Args...>::t);
    }
    virtual ~Event();
};

template <class Func, class... Args>
Event<Func,Args...>::~Event() {
}

template <typename T, typename... Args>
constexpr decltype (auto) make_event(T&& t, Args&& ...args) {
    return Event<typename std::decay<T>::type, std::shared_ptr<typename std::decay<Args>::type>...> (std::move(t),make_shared_tuple(std::move(args)...));
}

template <typename T, typename... Args>
constexpr decltype (auto) make_event_ptr(T&& t, Args&& ...args) {
    return std::make_unique<Event<typename std::decay<T>::type, std::shared_ptr<typename std::decay<Args>::type>...>>(std::move(t),make_shared_tuple(std::move(args)...));
}

template <typename T, typename... Args>
constexpr decltype (auto) make_event_base_ptr(T&& t, Args&& ...args) {
    return std::unique_ptr<EventBase>(new Event<typename std::decay<T>::type, std::shared_ptr<typename std::decay<Args>::type>...>(std::move(t),make_shared_tuple(std::move(args)...)));
}

template <class Tuple>
constexpr decltype (auto) sizeof_tuple(__attribute__((unused)) Tuple&& t) {
    return std::tuple_size<typename std::decay<Tuple>::type>::value;
}

template<class F, class...Args>
constexpr void for_each_arg(F&&f,Args&&...args){
  using discard=int[];
  (void)discard{0,((void)(
    f(std::forward<Args>(args))
  ),0)...};
}

template<size_t... Is, class Tuple, class F>
constexpr void for_each_tuple( std::index_sequence<Is...>, Tuple&& tup, F&& f ) {
  using std::get;
  for_each_arg(
    std::forward<F>(f),
    get<Is>(std::forward<Tuple>(tup))...
  );
}

template<class Tuple ,class... Args>
constexpr void update_task_values(EngineExperiments::TaskBase* task, Args& ...args){
    dynamic_cast<EngineExperiments::TaskVarTable<Tuple>*>(task)->getVarPointersTable() = std::tie(args...);
}

template<class... Args>
constexpr void set_task_values(EngineExperiments::TaskBase* task, Args&& ...args){
    dynamic_cast<EngineExperiments::TaskVarTable<typename std::decay<Args>::type...>*>(task)->getVarPointersTable() = std::make_tuple(std::forward<Args>(args)...);
}

}

template <class T>
void print(const T& item) {
    std::cout << item << " ";
}

template<class F, class...Args>
constexpr void for_each_arg(F&&f,Args&&...args){
  using discard=int[];
  (void)discard{0,((void)(
    f(std::forward<Args>(args))
  ),0)...};
}

template<size_t... Is, class Tuple, class F>
constexpr void for_each_tuple( std::index_sequence<Is...>, Tuple&& tup, F&& f ) {
  using std::get;
  for_each_arg(
    std::forward<F>(f),
    get<Is>(std::forward<Tuple>(tup))...
  );
}

template<class F, class...Args>
constexpr void for_each_args(F&&f,Args&&...args){
    f(std::forward<Args>(args)...);
}

template<size_t... Is, class F, class... Tuple>
constexpr void for_each_tuples( std::index_sequence<Is...> seq, F&& f, Tuple&&... tups) {
    using std::get;
    using discard=int[];
    (void)discard{0,((void)(
      for_each_tuple(seq,std::forward<Tuple>(tups),std::forward<F>(f))
    ),0)...};
}

template <class Tuple>
constexpr decltype (auto) sizeof_tuple(__attribute__((unused)) Tuple&& t) {
    return std::tuple_size<typename std::decay<Tuple>::type>::value;
}

std::vector<std::unique_ptr<EngineExperiments::EventBase>> event_pool;
std::mutex pool_lock,out_lock;
ThreadPool<20000,EngineExperiments::TaskBase*> pool(std::thread::hardware_concurrency(),std::thread::hardware_concurrency());

template<class T>
class Singleton {
    static std::shared_ptr<T> ptr;
public:
    template<class ...Args>
    static decltype (auto) getHandler (Args&&... args) {
        static std::once_flag once;
        std::call_once(once,[&] (Args&&... args) {
            Singleton<T>::ptr.reset(new T(std::forward<Args>(args)...));
        },std::forward<Args>(args)...);
        return ptr;
    }
};

template<class T> std::shared_ptr<T> Singleton<T>::ptr = nullptr;

int main() {
    {
        std::lock_guard<decltype (pool_lock)> lk(pool_lock);
        event_pool.emplace_back(EngineExperiments::make_event_base_ptr([]{
            EngineExperiments::EventBase* base_event;
            {
                std::lock_guard<decltype (pool_lock)> lk(pool_lock);
                event_pool.emplace_back(EngineExperiments::make_event_base_ptr([](const auto& item) {
                    std::lock_guard<decltype (out_lock)> lk(out_lock);
                    std::cout << item << std::endl;
                },0u));
                auto index = event_pool.size() - 1;
                base_event = event_pool[index].get();
            }
            EngineExperiments::TaskBase* task;
            for (auto i = 0u; i < 100u; ++i) {
                task = base_event->generateTask().release();
                EngineExperiments::set_task_values(task,i);
                pool.push(task,i % pool.queue_size());
            }
        }));
        auto index = event_pool.size() - 1;
        auto base_event = event_pool[index].get();
        auto task = base_event->generateTask();
        pool.push(task.release(),0);
    }
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1ms);
    pool.wait_for_empty();

}
