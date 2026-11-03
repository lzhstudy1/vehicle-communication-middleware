add_library(vcm_core STATIC 
     logger.cpp 
     can_frame.cpp 
     can_socket.cpp 
     signal_database.cpp
     isotp_sender.cpp
     IsoTpReceiver.cpp
)

target_include_directories(vcm_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

if(ENABLE_TSAN)
    target_compile_options(vcm_core PRIVATE -fsanitize=thread -g)
    target_link_options(vcm_core INTERFACE -fsanitize=thread)
endif()

if(ENABLE_COVERAGE)
    target_compile_options(vcm_core PRIVATE --coverage)
    target_link_options(vcm_core PRIVATE --coverage)
endif()

target_link_libraries(vcm_core PUBLIC 
    Threads::Threads 
    nlohmann_json::nlohmann_json
    )

target_compile_options(vcm_core PRIVATE 
 -Wall -Wextra -Wpedantic )