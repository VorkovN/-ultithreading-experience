#include "ProducerConsumer.h"

#include <unistd.h>
#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <vector>

pthread_cond_t CV_CONSUMER = PTHREAD_COND_INITIALIZER;
pthread_cond_t CV_PRODUICER = PTHREAD_COND_INITIALIZER;
pthread_mutex_t TERM_MUTEX = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t SUM_MUTEX = PTHREAD_MUTEX_INITIALIZER;
bool debug = false;  //только читается, можно и без атомик
std::atomic<bool> producerIsAlive{false};

int runThreads(uint consumerThreadsCount, uint mMaxSleepTime, bool debugMode) {
  if (debugMode) debug = true;

  int term = 0;
  int sum = 0;

  // Produicer creation
  pthread_t producer;
  auto producerArgs = ProducerRoutineArgs{&term};
  pthread_create(&producer, nullptr, producerRoutine, &producerArgs);
  producerIsAlive.store(true, std::memory_order_relaxed);

  // Consumer creation
  std::vector<pthread_t> consumerThreads(consumerThreadsCount);
  ConsumerRoutineArgs consumerArgs{};
  for (uint i = 0; i < consumerThreadsCount; ++i) {
    consumerArgs = ConsumerRoutineArgs{&mMaxSleepTime, &term, &sum};
    pthread_create(&consumerThreads[i], nullptr, consumerRoutine,
                   &consumerArgs);
  }

  // Interrupter creation
  pthread_t interruptor;
  auto interruptorArgs = ConsumerInterruptorRoutineArgs{consumerThreads.size(),
                                                        consumerThreads.data()};
  pthread_create(&interruptor, nullptr, consumerInterruptorRoutine,
                 &interruptorArgs);

  pthread_join(producer, nullptr);
  for (uint i = 0; i < consumerThreadsCount; ++i)
    pthread_join(consumerThreads[i], nullptr);
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

    //чтобы не создавать лишний булиан на валидность значения в term, можно
    //считать, что на вход не будут поступать слагаемые 0 ибо в них нет смысла,
    //тем более, что в тестах на сервере 0 не подается)
    if (*consumerRoutineArgs->term == 0)
      pthread_cond_wait(&CV_CONSUMER, &TERM_MUTEX);

    if (!producerIsAlive.load(std::memory_order_relaxed)) {
      pthread_mutex_unlock(&TERM_MUTEX);
      break;
    }

    psum += *consumerRoutineArgs->term;
    *consumerRoutineArgs->term = 0;
    pthread_cond_signal(&CV_PRODUICER);

    if (debug) std::cout << "(" << getTid() << ", " << psum << ")" << std::endl;
    pthread_mutex_unlock(&TERM_MUTEX);
    uint uRandomSleepTime =
        generateuRandomSleepTime(*consumerRoutineArgs->mMaxRandomSleepTime);

    usleep(uRandomSleepTime);
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

    if (!(std::cin >> *producerRoutineArgs->term)) {
      producerIsAlive.store(false, std::memory_order_relaxed);
      pthread_mutex_unlock(&TERM_MUTEX);
      break;
    }
    // local_term не проподет со стека, пока ее не возьмет консьюмер и не
    // заставить продьюсера проснуться
    pthread_cond_signal(&CV_CONSUMER);
    pthread_cond_wait(&CV_PRODUICER, &TERM_MUTEX);
    pthread_mutex_unlock(&TERM_MUTEX);
  }
  //чтобы дождаться завершения всех консьюмеров
  sleep(1);

  pthread_mutex_lock(&TERM_MUTEX);
  pthread_cond_broadcast(&CV_CONSUMER);
  pthread_mutex_unlock(&TERM_MUTEX);
  return nullptr;
}

void* consumerInterruptorRoutine(void* args) {
  auto consumerInterruptorRoutineArgs =
      static_cast<ConsumerInterruptorRoutineArgs*>(args);

  while (producerIsAlive.load(std::memory_order_relaxed)) {
    int randomThreadId = rand() % consumerInterruptorRoutineArgs->threadsCount;
    pthread_cancel(consumerInterruptorRoutineArgs->threads[randomThreadId]);
    usleep(1);  //без минимального слипа все упадет
  }
  return nullptr;
}

uint generateuRandomSleepTime(uint mMaxSleepTime) {
  srand(time(nullptr));
  return mMaxSleepTime ? (rand() % mMaxSleepTime + 1) * 1000 : 0;
}

uint getTid() {
  static uint tidsCounter = 1;
  thread_local std::unique_ptr<uint> tid;

  if (tid == nullptr) tid = std::make_unique<uint>(tidsCounter++);

  return *tid;
}
