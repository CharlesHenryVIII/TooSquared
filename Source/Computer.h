#pragma once
#include "Math.h"

#include <atomic>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>

struct Job
{
    virtual void DoThing() = 0;
};

struct MultiThreading {
private:
    std::mutex                              m_jobVectorMutex;
    std::counting_semaphore<PTRDIFF_MAX>    m_semaphore;
    std::atomic<int32>                      m_jobs_in_flight = {};
    std::atomic<bool>                       m_running;
    std::vector<Job*>                       m_jobs;
    std::vector<std::thread>                m_threads;

    static int32 ThreadFunction(void* data);
    MultiThreading();
    ~MultiThreading();
    MultiThreading(MultiThreading&) = delete;
    MultiThreading& operator=(MultiThreading&) = delete;
    Job* AcquireJob();

public:
    enum Threads : int32{
        single_thread,
        multi_thread,
    };
#if 1
    Threads threads = multi_thread;
#else
    Threads threads = single_thread;
#endif
    static MultiThreading& GetInstance()
    {
        static MultiThreading instance;
        return instance;
    }
    int32 GetJobsInFlight()
    {
        return m_jobs_in_flight;
    }
	void SubmitJob(Job* job);
};

bool OnMainThread();

