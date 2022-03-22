#pragma once

#include <pthread.h>

struct ConsumerRoutineArgs
{
  int* randomSleepTime;
  int* term;
  int* sum;
};

struct ConsumerInterruptorRoutineArgs
{
  int* randomSleepTime;
  size_t threadsCount;
  pthread_t* threads;
};

struct ProducerRoutineArgs
{
  int* randomSleepTime;
  int* term;
};

// the declaration of run threads can be changed as you like
int run_threads(int consumerThreadsCount, int maxSleepTime);
