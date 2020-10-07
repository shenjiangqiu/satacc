cmake_minimum_required(VERSION 3.10)
add_library(generate_wrap set_up.cc)
target_include_directories(generate_wrap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(generate_wrap PUBLIC mem_req_interface)