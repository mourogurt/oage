#include <thread_guard.hpp>
#include <iostream>
#include <QtTest>

class TestExecThread : public QObject {
    Q_OBJECT
private slots:
    void execEvent();
};

void TestExecThread::execEvent() {
    //TODO: Critical
}

QTEST_MAIN(TestExecThread)
#include "main.moc"
