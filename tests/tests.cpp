#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <ProducerConsumer.h>

//Можно, конечно, и просто метод runThreads() протестировать; но так тест полнее
//и интереснее
TEST_CASE("fullTest") {
  char cmd[] = "./posix 8000 10 < ./tests/test_data.txt | tail -n 1";
  FILE* file = popen(cmd, "r");
  if (file == nullptr) return;
  int sum = 0;
  int result = fscanf(file, "%i", &sum);
  if (result == -1) std::cout << "file scaning error" << std::endl;

  CHECK(sum == 3114160);
}

TEST_CASE("getTidTest") {
  std::vector<int> tids;
  for (int i = 1; i <= 20; ++i) {
    pthread_t thread;

    auto lambda = [](void* arg) -> void* {
      auto* tids = static_cast<std::vector<int>*>(arg);
      tids->push_back(getTid());
      return nullptr;
    };

    pthread_create(&thread, nullptr, lambda, &tids);
    pthread_join(thread, nullptr);
  }
  std::string realOutput = {tids.begin(), tids.end()};
  std::vector<int> tidsReference = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
  CHECK_EQ(tids, tidsReference);
}