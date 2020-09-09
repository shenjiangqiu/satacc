
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} TESTSOURCES)
add_executable(main_test ${TESTSOURCES})
target_link_libraries(main_test PUBLIC satacc)

add_library(generate_wrap INTERFACE set_up.h)
target_include_directories(generate_wrap INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})