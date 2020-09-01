cmake_minimum_required(VERSION 3.10)
project(SatAcc VERSION 0.0.1)
add_subdirectory(booksim2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

aux_source_directory(src SOURCES)


add_library(satacc STATIC ${SOURCES})
target_include_directories(satacc PUBLIC  
                    "${PROJECT_SOURCE_DIR}/include"
                     "$ENV{HOME}/usr/include" 
                     "$ENV{HOME}/git/DRAMsim3/ext/headers" 
                     "$ENV{HOME}/git/DRAMsim3/src")

target_link_directories(satacc PUBLIC "$ENV{HOME}/usr/lib"  
                                        "$ENV{HOME}/git/DRAMsim3")

target_link_libraries(satacc PUBLIC "fmt" "sjqcache" "dramsim3" )
target_link_libraries(satacc PRIVATE booksim2)

aux_source_directory(test TESTSOURCES)
add_executable(main_test ${TESTSOURCES})
target_link_libraries(main_test PUBLIC satacc)


add_executable(booksim_main booksim_main.cpp)
target_link_libraries(booksim_main booksim2)