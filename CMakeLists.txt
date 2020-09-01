cmake_minimum_required(VERSION 3.10)
project(SatAcc VERSION 0.0.1)
add_subdirectory(gpgpu_intersim)
add_subdirectory(sjqcache)
add_subdirectory(fmt)
add_subdirectory(DRAMsim3)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

aux_source_directory(src SOURCES)


add_library(satacc STATIC ${SOURCES})
target_include_directories(satacc PUBLIC  
                    "${CMAKE_CURRENT_SOURCE_DIR}/include"  
                     )

target_link_libraries(satacc PUBLIC fmt sjqcache dramsim3 minisat_utiles)
target_link_libraries(satacc PUBLIC intersim2)

aux_source_directory(test TESTSOURCES)
add_executable(main_test ${TESTSOURCES})
target_link_libraries(main_test PUBLIC satacc)

