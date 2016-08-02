# oage
Open Async Game Engine - Main goal of this engine: Create asynchronous event-based games
# roadmap
* Implement multi queue threadpool (WIP)
* Implement continuous scheduler (not started)
* Implement events dependencies (not started)
* Implement async allocator (not started)
* Implement random scheduler (not started)
* Implement clock timer, frame rate timer(not started)
* Implement event logic
* Etc...

# Unit-tests
* Event exec (mostly done)
* Thread guard test exec (mostly done)
* Exec threadpool when threads. num(tn) % queues. num(qn) == 0 and tn >= qn (started)
* Exec threadpool when qn % tn == 0 and  qn > tn (not started)
* Exec threadpool when qn % tn != 0 and qn > tn (not started)
* Exec threadpool when tn % qn != 0 and tn >= qn (not started)
