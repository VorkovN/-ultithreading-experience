#include "ProducerConsumer.h"

#include <csignal>
#include <iostream>
#include <vector>
#include <queue>

// можно было обойтись и без DECREMENT_MUTEX, но тогда код был бы менее читабельным
pthread_cond_t CV_CONSUMER = PTHREAD_COND_INITIALIZER;
pthread_cond_t CV_PRODUICER = PTHREAD_COND_INITIALIZER;
pthread_mutex_t TERM_MUTEX = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t SUM_MUTEX = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t TIDS_MUTEX = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t DECREMENT_MUTEX = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t READY_CONSUMERS_MUTEX = PTHREAD_MUTEX_INITIALIZER;
bool producerIsAlive = false;
uint readyConsumersCount = 0;
uint aliveConsumersCount = 0;

int runThreads(uint consumerThreadsCount, uint maxSleepTime) {
  int term = 0;
  int sum = 0;
  uint uRandomSleepTime;

  // Consumer creation
  std::vector<pthread_t> consumerThreads(consumerThreadsCount);
  ConsumerRoutineArgs consumerArgs{};
  for (int i = 0; i < consumerThreadsCount; ++i) {
    uRandomSleepTime = generateuRandomSleepTime(maxSleepTime);
    consumerArgs = ConsumerRoutineArgs{&uRandomSleepTime, &term, &sum};
    pthread_create(&consumerThreads[i], nullptr, consumerRoutine, &consumerArgs);
    pthread_detach(consumerThreads[i]);
    ++aliveConsumersCount;
  }
  sleep(1);

  // Produicer creation
  pthread_t producer;
  uRandomSleepTime = generateuRandomSleepTime(maxSleepTime);
  auto producerArgs = ProducerRoutineArgs{&term};
  pthread_create(&producer, nullptr, producerRoutine, &producerArgs);
  pthread_detach(producer);
  producerIsAlive = true;
  sleep(1);

  // Interrupter creation
  pthread_t interruptor;
  uRandomSleepTime = generateuRandomSleepTime(maxSleepTime);
  auto interruptorArgs = ConsumerInterruptorRoutineArgs{consumerThreads.size(), consumerThreads.data()};
  pthread_create(&interruptor, nullptr, consumerInterruptorRoutine, &interruptorArgs);
  pthread_join(interruptor, NULL);

  pthread_cond_destroy(&CV_PRODUICER);
  pthread_cond_destroy(&CV_CONSUMER);
  pthread_mutex_destroy(&TERM_MUTEX);
  pthread_mutex_destroy(&SUM_MUTEX);
  return sum;
}

void* consumerRoutine(void* args) {
  auto* consumerRoutineArgs = static_cast<ConsumerRoutineArgs*>(args);
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
  thread_local int psum;

  while (true) {
    pthread_mutex_lock(&TERM_MUTEX);

    pthread_mutex_lock(&READY_CONSUMERS_MUTEX);
    ++readyConsumersCount;
    pthread_mutex_unlock(&READY_CONSUMERS_MUTEX);

    pthread_cond_wait(&CV_CONSUMER, &TERM_MUTEX);

    pthread_mutex_lock(&READY_CONSUMERS_MUTEX);
    --readyConsumersCount;
    pthread_mutex_unlock(&READY_CONSUMERS_MUTEX);

    int term = *(consumerRoutineArgs->term);
    pthread_cond_signal(&CV_PRODUICER);
    pthread_mutex_unlock(&TERM_MUTEX);

    if (!producerIsAlive) {
      pthread_mutex_lock(&DECREMENT_MUTEX);
      --aliveConsumersCount;
      pthread_mutex_unlock(&DECREMENT_MUTEX);
      break;
    }

    psum += term;

#ifndef NDEBUG
    std::cout << "(" << getTid() << ", " << psum << ")" << std::endl;
#endif
    usleep(*consumerRoutineArgs->uRandomSleepTime);
  }

  pthread_mutex_lock(&SUM_MUTEX);
  (*consumerRoutineArgs->sum) += psum;
  pthread_mutex_unlock(&SUM_MUTEX);
  return nullptr;
}

void* producerRoutine(void* args) {
  auto producerRoutineArgs = static_cast<ProducerRoutineArgs*>(args);

  while (true) {
    //костыль, для гарантии, что хоть один консьюмер будет готов к
    //pthread_cond_signal
    if (readyConsumersCount == 0) {
      sleep(1);
      continue;
    }

    pthread_mutex_lock(&TERM_MUTEX);
    if (!(std::cin >> *(producerRoutineArgs->term))) {
      pthread_mutex_unlock(&TERM_MUTEX);
      break;
    }
    pthread_cond_signal(&CV_CONSUMER);
    pthread_cond_wait(&CV_PRODUICER, &TERM_MUTEX);
    pthread_mutex_unlock(&TERM_MUTEX);
  }

  //можно конечно проверять потоки на готовность словить broadcast и в случае
  //неготовности немного подождать,
  // но для данной задачи sleep(1) более чем достаточно
  sleep(1);

  producerIsAlive = false;
  pthread_mutex_lock(&TERM_MUTEX);
  pthread_cond_broadcast(&CV_CONSUMER);
  pthread_mutex_unlock(&TERM_MUTEX);
  return nullptr;
}

void* consumerInterruptorRoutine(void* args) {
  auto consumerInterruptorRoutineArgs =
      static_cast<ConsumerInterruptorRoutineArgs*>(args);

  while (aliveConsumersCount > 0) {
    if (producerIsAlive){
      int randomThreadId = rand() % consumerInterruptorRoutineArgs->threadsCount;
      pthread_cancel(consumerInterruptorRoutineArgs->threads[randomThreadId]);
    }
    usleep(10);
  }
  return nullptr;
}

uint generateuRandomSleepTime(uint maxSleepTime) {
  srand(time(nullptr));
  return (rand() % maxSleepTime + 2 + 1) * 1000;
}

uint getTid() {
  thread_local static uint* tid;
  static std::queue<uint> tids;

  pthread_mutex_lock(&TIDS_MUTEX);
  if (tid == nullptr)
  {
    tids.push(tids.size()+1);
    tid=&tids.back();
  }
  pthread_mutex_unlock(&TIDS_MUTEX);

  return *tid;
}
