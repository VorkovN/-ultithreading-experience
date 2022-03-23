#include "producer_consumer.h"

#include <iostream>
#include <vector>
#include <thread>
#include <csignal>

static pthread_cond_t  COND_VAR   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t TERM_MUTEX = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t SUM_MUTEX  = PTHREAD_MUTEX_INITIALIZER;


uint generateuRandomSleepTime(uint maxSleepTime){
  srand(time(nullptr));
  return (rand()%maxSleepTime+2+1)*1000;
}

int get_tid()
{
  // 1 to 3+N thread ID
  thread_local static int* tid;
  return *tid;
}

void* producer_routine(void* args) {

  auto* producerRoutineArgs = static_cast<ProducerRoutineArgs*>(args);

  bool result;
  while (result) {
    pthread_mutex_lock(&TERM_MUTEX);
    result = {std::cin >> *producerRoutineArgs->term};
    pthread_cond_signal(&COND_VAR);
    pthread_mutex_unlock(&TERM_MUTEX);
  }

  return nullptr;
}

void* consumer_routine(void* args) {

  auto* consumerRoutineArgs = static_cast<ConsumerRoutineArgs*>(args);
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

  while (true) {
    pthread_mutex_lock(&TERM_MUTEX);
    pthread_cond_wait(&COND_VAR, &TERM_MUTEX);
    int term = *consumerRoutineArgs->term;
    pthread_mutex_unlock(&TERM_MUTEX);

    pthread_mutex_lock(&SUM_MUTEX);
    (*consumerRoutineArgs->sum) += term;
    pthread_mutex_unlock(&SUM_MUTEX);

    usleep(*consumerRoutineArgs->uRandomSleepTime);
  }
  return nullptr;
}

void* consumer_interruptor_routine(void* args) {
  // interrupt random consumer while producer is running
  auto* consumerInterruptorRoutineArgs = static_cast<ConsumerInterruptorRoutineArgs*>(args);

  while (true) {
    for (int i = 0; i < consumerInterruptorRoutineArgs->threadsCount; ++i)
      pthread_cancel(consumerInterruptorRoutineArgs->threads[i]);
    usleep(*consumerInterruptorRoutineArgs->uRandomSleepTime);
  }
  return nullptr;
}

// the declaration of run threads can be changed as you like
int run_threads(uint consumerThreadsCount, uint maxSleepTime) {
  // start N threads and wait until they're done
  // return aggregated sum of values
  int term;
  int sum = 0;
  uint uRandomSleepTime;

  pthread_t producer;
  uRandomSleepTime = generateuRandomSleepTime(maxSleepTime);
  auto producerArgs = ProducerRoutineArgs{&term};
  pthread_create(&producer, nullptr, producer_routine, &producerArgs);
  pthread_detach(producer);

  std::vector<pthread_t> consumerThreads(consumerThreadsCount);
  for (int i = 0; i < consumerThreadsCount; ++i)
  {
    uRandomSleepTime = generateuRandomSleepTime(maxSleepTime);
    auto consumerArgs = ConsumerRoutineArgs{&uRandomSleepTime, &term, &sum};
    pthread_create(&consumerThreads[i], nullptr, consumer_routine, &consumerArgs);
    pthread_detach(consumerThreads[i]);
  }

  pthread_t interruptor;
  uRandomSleepTime = generateuRandomSleepTime(maxSleepTime);
  auto interruptorArgs = ConsumerInterruptorRoutineArgs{&uRandomSleepTime, consumerThreads.size(), consumerThreads.data()};
  pthread_create(&interruptor, nullptr, consumer_interruptor_routine, &interruptorArgs);
  void** interruptorResult;
  pthread_join(interruptor, interruptorResult);

  //TODO wait уничтожения всех потоков
  //TODO зацикливание потоков

  pthread_cond_destroy(&COND_VAR);
  pthread_mutex_destroy(&TERM_MUTEX);
  return 0;
}
