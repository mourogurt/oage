#include <thread_guard.hpp>
#include <iostream>
#include <QtTest>

class TestExecThread : public QObject {
    Q_OBJECT
private slots:
    void execEvent();
};

void TestExecThread::execEvent() {
    auto first (0u), second(0u);
    {
        thread_guard(std::thread([&first](){
            first = 50u;
        }),make_event_ptr_base([&second]() {
            second = 100u;
        }));
    }
    QCOMPARE(first,50u);
    QCOMPARE(second,100u);
}

QTEST_MAIN(TestExecThread)
#include "main.moc"
