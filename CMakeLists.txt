
cmake_minimum_required(VERSION 3.15)

project(TimeUUIDBenchmark VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -Wall -O2 -lbenchmark -march=native")
set(CMAKE_CXX_FLAGS_DEBUG   "-Wall -O2 -lbenchmark -march=native")

add_executable(benchmark benchmark.cc)
