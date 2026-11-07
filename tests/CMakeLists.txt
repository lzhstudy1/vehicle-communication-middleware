add_executable(vcm_tests
    test_logger.cpp
    test_signal_database.cpp
    test_isotp_sender.cpp
    test_ring_buffer.cpp
    test_Isotp_receive.cpp
    test_Uds_server.cpp
)

target_link_libraries(vcm_tests PRIVATE 
    vcm_core 
    GTest::gtest_main
)

if(ENABLE_COVERAGE)
    target_compile_options(vcm_tests PRIVATE --coverage -O0 -g)
    target_link_options(vcm_tests PRIVATE --coverage)
endif()

target_compile_options(vcm_tests PRIVATE 
    -Wall -Wextra -Wpedantic
)

target_compile_definitions(vcm_tests PRIVATE 
    TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/dbc"
)

add_test(NAME vcm_tests COMMAND vcm_tests)