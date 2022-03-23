#pragma once

#include <pthread.h>
#include <cstdlib>

struct ConsumerRoutineArgs
{
  uint* uRandomSleepTime;
  int* term;
  int* sum;
};

struct ConsumerInterruptorRoutineArgs
{
  uint* uRandomSleepTime;
  size_t threadsCount;
  pthread_t* threads;
};

struct ProducerRoutineArgs
{
  int* term;
};

// the declaration of run threads can be changed as you like
int run_threads(uint consumerThreadsCount, uint maxSleepTime);
