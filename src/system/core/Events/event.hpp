#ifndef EVENT_HPP
#define EVENT_HPP
#include <tuple>
#include <experimental/tuple>
#include <memory>
#include <future>

struct EventBase {
    EventBase() {}
    EventBase(const EventBase&) {}
    EventBase(EventBase&&) {}
    virtual void operator()() = 0;
    virtual ~EventBase();
};

EventBase::~EventBase() {
}

template <typename Func, typename Args>
struct Event : EventBase {
    Event() = delete;
    Event(const Func& foo, const Args& tuple) noexcept: event_func(foo), tuple(tuple) {}
    Event(Func&& foo, Args&& tuple) noexcept : event_func(std::move(foo)) , tuple(std::move(tuple)) {}
    Func event_func;
    Args tuple;
    virtual void operator() () override {
        std::experimental::apply(event_func,tuple);
    }

    virtual ~Event() override {
    }
};

template <typename T, typename... Args>
decltype (auto) make_event(T&& t, Args&& ...args) {
    return Event<typename std::decay<T>::type,std::tuple<typename std::decay<Args>::type...>>(std::move(t),std::make_tuple(std::move(args)...));
}

template <typename T, typename... Args>
decltype (auto) make_event_ptr(T&& t, Args&& ...args) {
    return std::make_unique<Event<typename std::decay<T>::type,std::tuple<typename std::decay<Args>::type...>>>
            (std::move(t),std::make_tuple(std::move(args)...));
}

template <typename T, typename... Args>
decltype (auto) make_event_ptr_base(T&& t, Args&& ...args) {
    return std::unique_ptr<EventBase>(new Event<typename std::decay<T>::type,std::tuple<typename std::decay<Args>::type...>>(std::move(t),std::make_tuple(std::move(args)...)));
}

#endif // EVENT_HPP
