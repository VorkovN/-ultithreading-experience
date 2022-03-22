#include "producer_consumer.h"

#include <iostream>
#include <vector>
#include <thread>

static pthread_mutex_t TERM_MUTEX = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t SUM_MUTEX = PTHREAD_MUTEX_INITIALIZER;

int generateRandomSleepTime(int threadsCount){
  srand(time(nullptr));
  return rand()%threadsCount+2+1;
}

int get_tid()
{
  // 1 to 3+N thread ID
  thread_local static int* tid;
  return *tid;
}

void* producer_routine(void* args) {
  // read data, loop through each value and update the value, notify consumer,
  // wait for consumer to process
  auto* producerRoutineArgs = static_cast<ProducerRoutineArgs*>(args);
  bool result;
  while (result)
  {
    pthread_mutex_lock(&TERM_MUTEX);
    result = {std::cin >> *producerRoutineArgs->term};
    pthread_mutex_unlock(&TERM_MUTEX);
  };
  return nullptr;
}

void* consumer_routine(void* args) {
  // for every update issued by producer, read the value and add to sum
  // return pointer to result (for particular consumer)
  auto* consumerRoutineArgs = static_cast<ConsumerRoutineArgs*>(args);
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

  pthread_mutex_lock(&TERM_MUTEX);
  int term = *consumerRoutineArgs->term;
  pthread_mutex_unlock(&TERM_MUTEX);

  pthread_mutex_lock(&TERM_MUTEX);
  (*consumerRoutineArgs->sum)+=term;
  pthread_mutex_unlock(&TERM_MUTEX);

  return nullptr;
}

void* consumer_interruptor_routine(void* args) {
  // interrupt random consumer while producer is running
  auto* consumerInterruptorRoutineArgs = static_cast<ConsumerInterruptorRoutineArgs*>(args);
  for (int i = 0; i < consumerInterruptorRoutineArgs->threadsCount; ++i)
  pthread_cancel(consumerInterruptorRoutineArgs->threads[i]);

  return nullptr;
}

// the declaration of run threads can be changed as you like
int run_threads(int consumerThreadsCount, int maxSleepTime) {
  // start N threads and wait until they're done
  // return aggregated sum of values
  int term;
  int sum = 0;
  int randomSleepTime;

  std::vector<pthread_t> consumerThreads(consumerThreadsCount);
  for (int i = 0; i < consumerThreadsCount; ++i)
  {
    randomSleepTime = generateRandomSleepTime(consumerThreadsCount);
    auto consumerArgs = ConsumerRoutineArgs{&randomSleepTime, &term, &sum};
    pthread_create(&consumerThreads[i], nullptr, consumer_routine, &consumerArgs);
    pthread_detach(consumerThreads[i]);
  }

  pthread_t interruptor;
  randomSleepTime = generateRandomSleepTime(consumerThreadsCount);
  auto interruptorArgs = ConsumerInterruptorRoutineArgs{&randomSleepTime, consumerThreads.size(), consumerThreads.data()};
  pthread_create(&interruptor, nullptr, consumer_interruptor_routine, &interruptorArgs);
  pthread_detach(interruptor);

  pthread_t producer;
  randomSleepTime = generateRandomSleepTime(consumerThreadsCount);
  auto producerArgs = ProducerRoutineArgs{&randomSleepTime, &term};
  pthread_create(&producer, nullptr, producer_routine, &producerArgs);
  pthread_detach(producer);

  //TODO wait уничтожения всех потоков
  //TODO join на interrupt?
  //TODO зацикливание потоков

  return 0;
}
