#pragma once
#include "Math.h"
#include "SDL\include\SDL.h"

#include <vector>

struct Job
{
    virtual void DoThing() = 0;
};

//
//struct SetVertexAO : public Job {
//    virtual void DoThing();
//};

struct MultiThreading {
private:
    SDL_mutex* m_jobVectorMutex = nullptr;
    SDL_sem* m_semaphore = nullptr;
    SDL_atomic_t m_jobs_in_flight = {};
    SDL_sem* m_wait_semaphore = nullptr;
    std::vector<Job*> m_jobs;
    std::vector<SDL_Thread*> m_threads;
    SDL_atomic_t running;

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
        return SDL_AtomicGet(&m_jobs_in_flight);
    }
	void SubmitJob(Job* job);
};

bool OnMainThread();
