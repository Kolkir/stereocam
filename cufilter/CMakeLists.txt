cmake_minimum_required(VERSION 2.8)
project(cu_filter)

find_package(CUDA REQUIRED)
set(CUDA_PROPAGATE_HOST_FLAGS OFF)
set(CUDA_HOST_COMPILATION_CPP OFF)
set(CUDA_HOST_COMPILER "")

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CUDA_NVCC_FLAGS "-std=c++11")
set(CUDA_NVCC_FLAGS_RELEASE "-O3")
set(CUDA_NVCC_FLAGS_DEBUG "-O0 -g")

file(GLOB CU_SOURCES "*.cpp" "*.h" "*.cu")

cuda_add_library(cu_filter SHARED ${CU_SOURCES})

