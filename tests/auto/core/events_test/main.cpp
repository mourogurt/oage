#include <event.hpp>
#include <QtTest>

class TestExecEvent : public QObject {
    Q_OBJECT
private slots:
    void execEvent();
};

template <typename T>
decltype (auto) make_unique_from_rvalue(T&& t) {
    return std::make_unique<T>(std::move(t));
}

void TestExecEvent::execEvent() {
    auto i = 0u;
    std::unique_ptr<EventBase> ptr(make_event_ptr_base(
                                       [](auto i) {
        *i = 100u;
    },&i));
    (*ptr)();
    QCOMPARE(i,100u);
}

QTEST_MAIN(TestExecEvent)
#include "main.moc"
