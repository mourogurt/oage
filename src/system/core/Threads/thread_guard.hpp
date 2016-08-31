#ifndef THREAD_GUARD_HPP
#define THREAD_GUARD_HPP
#include <event.hpp>
#include <thread>

/*class thread_guard
{
    std::thread t;
    std::unique_ptr<EventBase> ev {nullptr};
public:
    explicit thread_guard(std::thread&& t_, EventBase&& ev_) noexcept : t(std::move(t_)), ev(&ev_) {}
    explicit thread_guard(std::thread&& t_, std::unique_ptr<EventBase>&& ev_) noexcept : t(std::move(t_)), ev(std::move(ev_)) {}
    explicit thread_guard(std::thread&& t_) noexcept : t(std::move(t_)) {}
    thread_guard(thread_guard const&) = delete;
    thread_guard& operator=(thread_guard const&) = delete;
    std::thread& operator &() noexcept {
        return t;
    }
    std::thread* operator *() noexcept {
        return &t;
    }
    ~thread_guard()
    {
        if (t.joinable()) {
            if (ev != nullptr)
                (*ev)();
            t.join();
        }
    }
};*/

#endif // THREAD_GUARD_HPP
