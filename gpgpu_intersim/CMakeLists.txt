cmake_minimum_required(VERSION 3.18)
project(intersim2)
file(GLOB SOURCES *.cpp  allocators/*.cpp arbiters/*.cpp networks/*.cpp power/*.cpp routers/*.cpp)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(BISON REQUIRED)
BISON_TARGET(Y config.y ${CMAKE_CURRENT_BINARY_DIR}/y.tab.c)

find_package(FLEX REQUIRED)
FLEX_TARGET(F config.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_library(intersim2 ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c ${CMAKE_CURRENT_BINARY_DIR}/y.tab.c)
target_include_directories(intersim2 PUBLIC 
                            "${CMAKE_CURRENT_SOURCE_DIR}"
                            "${CMAKE_CURRENT_SOURCE_DIR}/allocators"
                            "${CMAKE_CURRENT_SOURCE_DIR}/arbiters"
                            "${CMAKE_CURRENT_SOURCE_DIR}/examples"
                            "${CMAKE_CURRENT_SOURCE_DIR}/networks"
                            "${CMAKE_CURRENT_SOURCE_DIR}/power"
                            "${CMAKE_CURRENT_SOURCE_DIR}/routers")   
                           
target_include_directories(intersim2 PUBLIC ${CMAKE_CURRENT_BINARY_DIR})       