#ifndef CU_MEDIAN_H
#define CU_MEDIAN_H

#include <vector>
#include <queue>
#include <cstddef>

namespace cuda {

class Median3DFilter final {
  public:
    Median3DFilter(size_t width, size_t height, size_t blockSize, size_t depth);
    ~Median3DFilter();

    void pushFrame(const unsigned char *data);

    void getFilteredFrame(unsigned char* data);

    size_t getWidth() const;
    size_t getHeight() const;

  private:
    size_t width;
    size_t height;
    size_t depth;
    size_t blockSize;
    size_t frameBytesNumber;
    unsigned char* frames;
    unsigned char* cudaResult;
    std::queue<size_t> framesQueue;
};

}

#endif // CU_MEDIAN_H
