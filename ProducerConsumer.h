#ifndef PRODUCERCONSUMER
#define PRODUCERCONSUMER

#include <pthread.h>
#include <cstdlib>

struct ConsumerRoutineArgs {
  uint* uRandomSleepTime;
  int* term;
  int* sum;
};

struct ConsumerInterruptorRoutineArgs {
  size_t threadsCount;
  pthread_t* threads;
};

struct ProducerRoutineArgs {
  int* term;
};

int runThreads(uint consumerThreadsCount, uint maxSleepTime, bool debugMode);

void* consumerRoutine(void* args);

void* producerRoutine(void* args);

void* consumerInterruptorRoutine(void* args);

uint generateuRandomSleepTime(uint maxSleepTime);

uint getTid();

#endif
