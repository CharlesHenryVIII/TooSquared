#include "Computer.h"
#include "Math.h"
#include "WinInterop.h"
#include "SDL\include\SDL.h"

uint32 UsableCores()
{
    return Max<uint32>(1, SDL_GetCPUCount() - 1);
}

MultiThreading::MultiThreading()
    : m_semaphore(0)
{
    m_running = 1;
    uint32 usableCores = UsableCores();
    for (uint32 i = 0; i < usableCores; ++i)
    {
        m_threads.push_back(std::thread(&MultiThreading::ThreadFunction, nullptr));
        SetThreadName(m_threads[i].native_handle(), ToString("Thread %s", std::to_string(i).c_str()).c_str());
        DebugPrint("Created New Thread: %i\n", i);
    }
}
MultiThreading::~MultiThreading()
{
    m_running = false;
    m_semaphore.release(m_threads.size());

    for (std::thread& thread : m_threads)
        thread.join();
}

[[nodiscard]] Job* MultiThreading::AcquireJob()
{
    
    std::lock_guard<std::mutex> lock(m_jobVectorMutex);
    if (m_jobs.empty())
        return nullptr;
    m_jobs_in_flight++;

    Job* job = m_jobs[0];
    m_jobs.erase(m_jobs.begin());
    return job;
}

void MultiThreading::SubmitJob(Job* job)
{
    switch (threads)
    {
    case Threads::single_thread:

        job->DoThing();
        delete job;
        break;
    case Threads::multi_thread:

        std::lock_guard<std::mutex> lock(m_jobVectorMutex);
        m_jobs.push_back(job);
        m_semaphore.release();
        break;
    }
}

int32 MultiThreading::ThreadFunction(void* data)
{
    MultiThreading& MT = GetInstance();

    while (true)
    {
        MT.m_semaphore.acquire();
        if (!MT.m_running)
            break;

        Job* job = MT.AcquireJob();
        assert(job);
        if (job == nullptr)
            continue;

        {
            //PROFILE_SCOPE("THREAD JOB: ");
            job->DoThing();
        }

        MT.m_jobs_in_flight--;
        delete job;
    }
    return 0;
}

std::thread::id mainThreadID = std::this_thread::get_id();
bool OnMainThread()
{
    return mainThreadID == std::this_thread::get_id();
}


