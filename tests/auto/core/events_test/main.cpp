#include <event.hpp>
#include <QtTest>

class TestExecEvent : public QObject {
    Q_OBJECT
private slots:
    void execEvent();
};

void TestExecEvent::execEvent() {
    //TODO: Critical
}

QTEST_MAIN(TestExecEvent)
#include "main.moc"
