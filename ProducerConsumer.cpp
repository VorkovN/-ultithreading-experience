#include "ProducerConsumer.h"

#include <unistd.h>
#include <csignal>
#include <iostream>
#include <queue>
#include <vector>
#include <memory>

// можно было обойтись и без DECREMENT_MUTEX, но тогда код был бы менее
// читабельным
pthread_cond_t CV_CONSUMER = PTHREAD_COND_INITIALIZER;
pthread_cond_t CV_PRODUICER = PTHREAD_COND_INITIALIZER;
pthread_mutex_t TERM_MUTEX = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t SUM_MUTEX = PTHREAD_MUTEX_INITIALIZER;
bool debug = false;
bool producerIsAlive = false;
uint readyConsumersCount = 0;
uint aliveConsumersCount = 0;

int runThreads(uint consumerThreadsCount, uint maxSleepTime, bool debugMode) {
  if (debugMode) debug = true;

  int term = 0;
  int sum = 0;
  uint uRandomSleepTime;

  // Consumer creation
  std::vector<pthread_t> consumerThreads(consumerThreadsCount);
  ConsumerRoutineArgs consumerArgs{};
  for (uint i = 0; i < consumerThreadsCount; ++i) {
    uRandomSleepTime = generateuRandomSleepTime(maxSleepTime);
    consumerArgs = ConsumerRoutineArgs{&uRandomSleepTime, &term, &sum};
    pthread_create(&consumerThreads[i], nullptr, consumerRoutine,
                   &consumerArgs);
    pthread_detach(consumerThreads[i]);
    ++aliveConsumersCount;
  }
  sleep(1);

  // Produicer creation
  pthread_t producer;
  auto producerArgs = ProducerRoutineArgs{&term};
  pthread_create(&producer, nullptr, producerRoutine, &producerArgs);
  pthread_detach(producer);
  producerIsAlive = true;

  // Interrupter creation
  pthread_t interruptor;
  auto interruptorArgs = ConsumerInterruptorRoutineArgs{consumerThreads.size(),
                                                        consumerThreads.data()};
  pthread_create(&interruptor, nullptr, consumerInterruptorRoutine,
                 &interruptorArgs);
  pthread_join(interruptor, nullptr);

  pthread_cond_destroy(&CV_CONSUMER);
  pthread_cond_destroy(&CV_PRODUICER);
  pthread_mutex_destroy(&TERM_MUTEX);
  pthread_mutex_destroy(&SUM_MUTEX);

  return sum;
}

void* consumerRoutine(void* args) {
  auto* consumerRoutineArgs = static_cast<ConsumerRoutineArgs*>(args);
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
  thread_local int psum = 0;

  while (true) {
    pthread_mutex_lock(&TERM_MUTEX);

    if (readyConsumersCount == 0) pthread_cond_signal(&CV_PRODUICER);

    ++readyConsumersCount;
    pthread_cond_wait(&CV_CONSUMER, &TERM_MUTEX);
    --readyConsumersCount;

    if (!producerIsAlive) {
      --aliveConsumersCount;
      pthread_mutex_unlock(&TERM_MUTEX);
      break;
    }

    psum += *consumerRoutineArgs->term;
    pthread_cond_signal(&CV_PRODUICER);

    static int counter = 1;
    if (debug)
      std::cout << counter++ << "(" << getTid() << ", " << psum << ")"
                << std::endl;

    pthread_mutex_unlock(&TERM_MUTEX);

    usleep(*consumerRoutineArgs->uRandomSleepTime);
  }

  pthread_mutex_lock(&SUM_MUTEX);
  *consumerRoutineArgs->sum += psum;
  pthread_mutex_unlock(&SUM_MUTEX);
  return nullptr;
}

void* producerRoutine(void* args) {
  auto producerRoutineArgs = static_cast<ProducerRoutineArgs*>(args);
  while (true) {
    pthread_mutex_lock(&TERM_MUTEX);
    if (readyConsumersCount == 0) pthread_cond_wait(&CV_PRODUICER, &TERM_MUTEX);

    if (!(std::cin >> *producerRoutineArgs->term)) {
      producerIsAlive = false;
      pthread_mutex_unlock(&TERM_MUTEX);
      break;
    }
    pthread_cond_signal(&CV_CONSUMER);
    pthread_cond_wait(&CV_PRODUICER, &TERM_MUTEX);
    pthread_mutex_unlock(&TERM_MUTEX);
  }

  sleep(1);

  pthread_mutex_lock(&TERM_MUTEX);
  pthread_cond_broadcast(&CV_CONSUMER);
  pthread_mutex_unlock(&TERM_MUTEX);
  return nullptr;
}

void* consumerInterruptorRoutine(void* args) {
  auto consumerInterruptorRoutineArgs =
      static_cast<ConsumerInterruptorRoutineArgs*>(args);

  while (aliveConsumersCount > 0) {
    if (producerIsAlive) {
      int randomThreadId =
          rand() % consumerInterruptorRoutineArgs->threadsCount;
      pthread_cancel(consumerInterruptorRoutineArgs->threads[randomThreadId]);
    }
    usleep(100000);
  }
  return nullptr;
}

uint generateuRandomSleepTime(uint maxSleepTime) {
  srand(time(nullptr));
  return maxSleepTime ? (rand() % maxSleepTime + 1) * 1000 : 0;
}

uint getTid() {
  static uint tidsCounter = 1;
  thread_local std::unique_ptr<uint> tid;

  if (tid == nullptr)
    tid = std::make_unique<uint>(tidsCounter++);

  return *tid;
}
