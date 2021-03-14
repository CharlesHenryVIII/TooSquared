#include "Computer.h"
#include "Math.h"
#include "WinInterop.h"

#include <thread>

uint32 UsableCores()
{
    return Max<uint32>(1, SDL_GetCPUCount() - 1);
}

MultiThreading::MultiThreading()
{
    m_semaphore = SDL_CreateSemaphore(0);
    m_jobVectorMutex = SDL_CreateMutex();
    m_wait_semaphore = SDL_CreateSemaphore(0);

    SDL_AtomicSet(&running, 1);
    uint32 usableCores = UsableCores();
    for (uint32 i = 0; i < usableCores; ++i)
    {
        m_threads.push_back(SDL_CreateThread(ThreadFunction, ("Thread " + std::to_string(i)).c_str(), nullptr));
        DebugPrint("Created New Thread: %i\n", i);
    }
}

[[nodiscard]] Job* MultiThreading::AcquireJob()
{
    SDL_LockMutex(m_jobVectorMutex);
    if (m_jobs.empty())
        return nullptr;
    SDL_AtomicAdd(&m_jobs_in_flight, 1);

    Job* job = m_jobs[0];
    m_jobs.erase(m_jobs.begin());
    SDL_UnlockMutex(m_jobVectorMutex);
    return job;
}

void MultiThreading::SubmitJob(Job* job)
{
#if 1
    SDL_LockMutex(m_jobVectorMutex);
    m_jobs.push_back(job);
    SDL_SemPost(m_semaphore);
    SDL_UnlockMutex(m_jobVectorMutex);
#else
    job->DoThing();
    delete job;
#endif
}

int32 MultiThreading::ThreadFunction(void* data)
{
    MultiThreading& MT = GetInstance();

    while (true)
    {
        int32 sem_result = SDL_SemWait(MT.m_semaphore);
        if (sem_result != 0 || SDL_AtomicGet(&MT.running) == 0)
            break;

        Job* job = MT.AcquireJob();
        assert(job);
        if (job == nullptr)
            continue;

        //Actual Job:
        {
            //PROFILE_SCOPE("THREAD JOB: ");
            job->DoThing();
        }

        int32 atomic_result = SDL_AtomicAdd(&MT.m_jobs_in_flight, -1);
        if (atomic_result == 1)
            SDL_SemPost(MT.m_wait_semaphore);
        delete job;
    }
    return 0;
}

std::thread::id mainThreadID = std::this_thread::get_id();

bool OnMainThread()
{
    return mainThreadID == std::this_thread::get_id();
}


