#include <iostream>
#include "ProducerConsumer.h"

int main(int argc, char** argv) {
  uint threadsCount = atoi(argv[1]);
  uint maxSleepTime = atoi(argv[2]);
  int sum = runThreads(threadsCount, maxSleepTime);
  std::cout << sum << std::endl;
  return 0;
}
