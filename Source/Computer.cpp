#include "Computer.h"
#include "Math.h"
#include "WinInterop.h"

uint32 ComputerSpecs::UsableCores()
{
	return Max<uint32>(1, coreCount - 1);
}

[[nodiscard]] Job* MultiThreading::AcquireJob()
{
    SDL_LockMutex(mutex);
    if (jobs.empty())
        return nullptr;
    SDL_AtomicAdd(&g_jobHandler.jobs_in_flight, 1);

    Job* job = jobs[0];
    jobs.erase(jobs.begin());
    SDL_UnlockMutex(mutex);
    return job;
}


int32 ThreadFunction(void* data)
{
    ThreadData* passedData = (ThreadData*)data;
    //const char* threadName = SDL_GetThreadName(this);

    while (true)
    {
        int32 sem_result = SDL_SemWait(g_jobHandler.semaphore);
        if (sem_result != 0 || SDL_AtomicGet(&passedData->running) == 0)
			break;
        DebugPrint("Thread Started\n");

        Job* job = g_jobHandler.AcquireJob();
        assert(job);
        if (job == nullptr)
            continue;
        DebugPrint("Job Started\n");

        //Actual Job:
        {
			job->chunk->flags.value |= CHUNK_LOADING;
			job->chunk->SetBlocks();
			job->chunk->BuildChunkVertices();
        }

        DebugPrint("Job Finished\n");
        int atomic_result = SDL_AtomicAdd(&g_jobHandler.jobs_in_flight, -1);
        if (atomic_result == 1)
            SDL_SemPost(g_jobHandler.wait_semaphore);
        delete job;
        DebugPrint("Job Deleted\n");
    }
    return 0;
}


ComputerSpecs g_computerSpecs;
MultiThreading g_jobHandler;
