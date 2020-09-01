cmake_minimum_required(VERSION 3.18)
project(Booksim2)
file(GLOB SOURCES *.cpp *.c allocators/*.cpp arbiters/*.cpp networks/*.cpp power/*.cpp routers/*.cpp)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_library(booksim2 ${SOURCES})
target_include_directories(booksim2 PUBLIC 
                            "${CMAKE_CURRENT_SOURCE_DIR}"
                            "${CMAKE_CURRENT_SOURCE_DIR}/allocators"
                            "${CMAKE_CURRENT_SOURCE_DIR}/arbiters"
                            "${CMAKE_CURRENT_SOURCE_DIR}/examples"
                            "${CMAKE_CURRENT_SOURCE_DIR}/networks"
                            "${CMAKE_CURRENT_SOURCE_DIR}/power"
                            "${CMAKE_CURRENT_SOURCE_DIR}/routers")           