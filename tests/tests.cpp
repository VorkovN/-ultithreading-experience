#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <ProducerConsumer.h>
#include <doctest.h>

//Можно, конечно, и просто метод runThreads() протестировать; но так тест полнее
//и интереснее
TEST_CASE("TestFull") {
  int buildResult = system("make all");
  if (buildResult == -1) std::cout << "make fail" << std::endl;

  char cmd[] = "./posix 500 10 < ./tests/test_data.txt | tail -n 1";
  FILE* file = popen(cmd, "r");
  if (file == nullptr) return;
  int sum = 0;
  int result = fscanf(file, "%i", &sum);
  if (result == -1) std::cout << "file scaning error" << std::endl;

  CHECK(sum == 3114160);
}

// TEST_CASE("TestRunThreads") {
//   uint threadsCount = 1000;
//   uint maxSleepTime = 10;
//   int sum = runThreads(threadsCount, maxSleepTime);
//   CHECK(sum == 3114160);
// }

TEST_CASE("TestGetTid") {
  std::set<int> tids;
  for (int i = 1; i <= 20; ++i) {
    pthread_t thread;

    auto lambda = [](void* arg) -> void* {
      auto* tids = static_cast<std::set<int8_t>*>(arg);
      tids->insert(getTid());
      return nullptr;
    };

    pthread_create(&thread, nullptr, lambda, &tids);
    pthread_join(thread, nullptr);
  }

  std::set<int> tidsReference = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

  //эквивалентность всех элементов в сете в нашем тесте можно утвержать по
  //совпадению крайних элементов и размера
  CHECK_EQ(*tids.begin(), *tidsReference.begin());
  CHECK_EQ(*tids.rbegin(), *tidsReference.rbegin());
  CHECK_EQ(tids.size(), tidsReference.size());
}
