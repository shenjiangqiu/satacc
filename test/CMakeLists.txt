
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} TESTSOURCES)
add_executable(main_test ${TESTSOURCES})
configure_file(DDR4-config.cfg DDR4-config.cfg COPYONLY)
target_link_libraries(main_test PUBLIC satacc generate_wrap)

