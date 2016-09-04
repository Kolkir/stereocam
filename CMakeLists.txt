cmake_minimum_required(VERSION 3.0)
project(stereocam)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# Tell CMake to run uic when necessary:
set(CMAKE_AUTOUIC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)

set(OpenCV_STATIC ON)
find_package(OpenCV REQUIRED)

#find_package (VTK REQUIRED NO_MODULE)
find_package (PCL REQUIRED)


include_directories (${PCL_INCLUDE_DIRS})
link_directories    (${PCL_LIBRARY_DIRS})
add_definitions     (${PCL_DEFINITIONS})

if(MSVC)
    set(CMAKE_CXX_FLAGS "/W4 /openmp /EHsc")
    set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1 /MP2")
    set(CMAKE_CXX_FLAGS_MINSIZEREL     "/MT /O1 /Ob1 /D NDEBUG /MP")
    set(CMAKE_CXX_FLAGS_RELEASE        "/MT /O2 /Ob2 /D NDEBUG /MP")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /Zi /O2 /Ob1 /D NDEBUG /MP")
else()
    SET(CMAKE_CXX_FLAGS "-Wall -std=c++11 -fopenmp")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g -DDEBUG ")
endif()

file(GLOB SRC_FILES src/*
    "*.h"
    "*.cpp"
    "*.ui"
)
add_executable(stereocam ${SRC_FILES})
target_link_libraries(stereocam ${OpenCV_LIBS} ${PCL_LIBRARIES})# ${VTK_LIBRARIES})
qt5_use_modules(stereocam Widgets)


