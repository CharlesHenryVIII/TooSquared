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

#if SOFA == 1
void SetBlocks::DoThing()
{
	//PROFILE_SCOPE("THREAD: SetBlocks()");
    g_chunks->SetBlocks(chunk);
    g_chunks->flags[chunk] |= CHUNK_LOADED_BLOCKS;
	g_chunks->flags[chunk] &= ~(CHUNK_LOADING_BLOCKS);
}

void CreateVertices::DoThing()
{
	//PROFILE_SCOPE("THREAD: CreateVertices()");
	g_chunks->BuildChunkVertices(chunk);
	g_chunks->flags[chunk] |= CHUNK_LOADED_VERTEX;
	g_chunks->flags[chunk] &= ~(CHUNK_LOADING_VERTEX);
}
#else
void SetBlocks::DoThing()
{
	PROFILE_SCOPE("THREAD: SetBlocks()");
	chunk->SetBlocks();
	chunk->flags |= CHUNK_BLOCKSSET;
}

void CreateVertices::DoThing()
{
	PROFILE_SCOPE("THREAD: CreateVertices()");
	chunk->flags |= CHUNK_LOADING;
	chunk->BuildChunkVertices();
	chunk->flags &= ~(CHUNK_LOADING);
	chunk->flags |= CHUNK_LOADED;
}
#endif

int32 ThreadFunction(void* data)
{
    ThreadData* passedData = (ThreadData*)data;
    //const char* threadName = SDL_GetThreadName(this);

    while (true)
    {
        int32 sem_result = SDL_SemWait(g_jobHandler.semaphore);
        if (sem_result != 0 || SDL_AtomicGet(&passedData->running) == 0)
			break;

        Job* job = g_jobHandler.AcquireJob();
        assert(job);
        if (job == nullptr)
            continue;

        //Actual Job:
        {
			//PROFILE_SCOPE("THREAD JOB: ");
            job->DoThing();
        }

        int32 atomic_result = SDL_AtomicAdd(&g_jobHandler.jobs_in_flight, -1);
        if (atomic_result == 1)
            SDL_SemPost(g_jobHandler.wait_semaphore);
        delete job;
    }
    return 0;
}


ComputerSpecs g_computerSpecs;
MultiThreading g_jobHandler;
