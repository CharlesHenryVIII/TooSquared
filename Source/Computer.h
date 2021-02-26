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

struct Job
{
    Chunk* chunk;
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

