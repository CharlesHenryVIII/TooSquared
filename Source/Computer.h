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
    std::mutex* m_jobVectorMutex = nullptr;
    std::counting_semaphore<PTRDIFF_MAX>* m_semaphore = nullptr;
    std::atomic<int32> m_jobs_in_flight = {};
    std::atomic<bool> running;
    std::vector<Job*> m_jobs;
    std::vector<std::thread> m_threads;

    static int32 ThreadFunction(void* data);
    MultiThreading();
    MultiThreading(MultiThreading&) = delete;
    MultiThreading& operator=(MultiThreading&) = delete;
    Job* AcquireJob();

public:
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
