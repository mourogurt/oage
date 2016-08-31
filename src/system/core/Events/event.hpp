#ifndef EVENT_HPP
#define EVENT_HPP
#include <tuple>
#include <experimental/tuple>
#include <memory>
#include <mutex>

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
    return Event<typename std::decay<T>::type, std::shared_ptr<typename std::decay<Args>::type>...>
            (std::move(t),std::make_tuple(std::make_shared<typename std::decay<Args>::type...>(std::move(args))...));
}

template <typename T, typename... Args>
constexpr decltype (auto) make_event_ptr(T&& t, Args&& ...args) {
    return std::make_unique<Event<typename std::decay<T>::type, std::shared_ptr<typename std::decay<Args>::type>...>>
                             (std::move(t),std::make_tuple(std::make_shared<typename std::decay<Args>::type...>(std::move(args))...));
}

template <typename T, typename... Args>
constexpr decltype (auto) make_event_base_ptr(T&& t, Args&& ...args) {
    return std::unique_ptr<EventBase>(new Event<typename std::decay<T>::type, std::shared_ptr<typename std::decay<Args>::type>...>
                                      (std::move(t),std::make_tuple(std::make_shared<typename std::decay<Args>::type...>(std::move(args))...)));
}

template<class... Args>
constexpr void set_task_values(TaskBase* task, Args&& ...args){
    dynamic_cast<TaskVarTable<typename std::decay<Args>::type...>*>(task)->getVarPointersTable() = std::make_tuple(std::forward<Args>(args)...);
}

#endif // EVENT_HPP
