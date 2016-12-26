#ifndef CU_UTILS_H
#define CU_UTILS_H

#include <chrono>
#include <string>
#include <iostream>
#include <cuda_runtime.h>

#define CHECK(call)                                                      \
{                                                                        \
   const cudaError_t error = call;                                       \
   if (error != cudaSuccess)                                             \
   {                                                                     \
      printf("Error: %s:%d, ", __FILE__, __LINE__);                      \
      printf("code:%d, reason: %s\n", error, cudaGetErrorString(error)); \
      exit(1);                                                           \
   }                                                                     \
}


class Timer {
  public:
    Timer(const std::string& name)
        : name(name) {
        start = std::chrono::steady_clock::now();
    }
    ~Timer() {
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(diff);
        diff -= sec;
        auto mil = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        std::cout << name << " time ";
        if (sec.count() > 0) {
            std::cout << sec.count() << " s ";
        }
        if (mil.count() > 0) {
            std::cout << mil.count() << " ms";
        }
        std::cout << std::endl;
    }

  private:
    std::string name;
    std::chrono::time_point<std::chrono::steady_clock> start;
};

#endif // CU_UTILS_H
