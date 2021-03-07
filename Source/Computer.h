#pragma once
#include "Math.h"
#include "SDL\include\SDL.h"
#include "Block.h"

#include <vector>

struct ComputerSpecs {
    uint32 coreCount = 0;
	uint32 UsableCores();
};

struct ThreadData {
    SDL_atomic_t running = {};

};

#if SOFA == 1
struct Job
{
    ChunkIndex chunk;
	virtual void DoThing() = 0;
};
#else
struct Job
{
    Chunk* chunk;
	virtual void DoThing() = 0;
};
#endif

struct SetBlocks : public Job {
	virtual void DoThing();
};

struct CreateVertices : public Job {
	virtual void DoThing();
};

struct MultiThreading {
	SDL_mutex* mutex = nullptr;
	SDL_sem* semaphore = nullptr;
	SDL_atomic_t jobs_in_flight = {};
	SDL_sem* wait_semaphore = nullptr;
	std::vector<Job*> jobs;
    std::vector<SDL_Thread*> threads;

	Job* AcquireJob();
};

int32 ThreadFunction(void* data);

extern ComputerSpecs g_computerSpecs;
extern MultiThreading g_jobHandler;

