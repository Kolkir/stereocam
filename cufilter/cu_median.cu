#include "cu_median.h"
#include "cu_utils.h"

#include <cassert>

namespace cuda {

Median3DFilter::Median3DFilter(size_t width, size_t height, size_t blockSize, size_t depth)
    : width(width)
    , height(height)
    , depth(depth)
    , blockSize(blockSize)
    , frameBytesNumber(width * height){
    assert(depth >= 1);
    assert(width >= 1 && height >= 1);
    CHECK(cudaMalloc(&frames, frameBytesNumber * depth));
    CHECK(cudaMalloc(&cudaResult, frameBytesNumber));
    framesQueue.push(0);
}

Median3DFilter::~Median3DFilter()
{
    CHECK(cudaFree(frames));
    CHECK(cudaFree(cudaResult));
}

void Median3DFilter::pushFrame(const unsigned char *data)
{
    size_t frameIndex = 0;
    if (framesQueue.size() == depth){
        frameIndex = framesQueue.front();
        framesQueue.pop();
    } else {
        frameIndex = framesQueue.back() + 1;
    }

    CHECK(cudaMemcpy(frames + (frameIndex * frameBytesNumber), data, frameBytesNumber, cudaMemcpyHostToDevice));

    framesQueue.push(frameIndex);
}

__global__ void medianOnGPU(unsigned char* frames, unsigned char* result,
                            size_t width, size_t height, size_t depth,
                            int searchNum,
                            size_t frameSize) {
    unsigned int ix = threadIdx.x + blockIdx.x * blockDim.x;
    unsigned int iy = threadIdx.y + blockIdx.y * blockDim.y;
    unsigned int iz = threadIdx.z + blockIdx.z * blockDim.z;//blockIdx.z - should be always zero
    if (ix < width && iy < height && iz < depth) {
        unsigned int idx = iy * width + ix;
        iz *= frameSize;
        //search element position in sorted array
        int pos = 0;
        for (size_t i = 0; i < depth; ++i){
            if (i != iz && frames[iz + idx] > frames[i * frameSize + idx]){
                ++pos;
            }
        }

        if (pos == searchNum){
            result[idx] = frames[iz + idx];
        }
    }
}

void Median3DFilter::getFilteredFrame(unsigned char *data)
{
    dim3 block (blockSize, blockSize, depth);
    //std::cout << "Block size : " << block.x << " " << block.y << " " << block.z << std::endl;

    dim3 grid  ((width  + block.x - 1) / block.x,
                (height + block.y - 1) / block.y,
                (depth  + block.z - 1) / block.z);    
    //std::cout << "Grid size : " << grid.x << " " << grid.y << " " << grid.z << std::endl;

    medianOnGPU<<< grid, block >>>(frames, cudaResult, width, height, depth, depth / 2, frameBytesNumber);

    CHECK(cudaPeekAtLastError());
    CHECK(cudaDeviceSynchronize());

    CHECK(cudaMemcpy(data, cudaResult, frameBytesNumber, cudaMemcpyDeviceToHost));
}

size_t Median3DFilter::getWidth() const {
    return width;
}

size_t Median3DFilter::getHeight() const {
    return height;
}

}
