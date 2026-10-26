add_library(vcm_core STATIC 
     logger.cpp 
     can_frame.cpp 
     can_socket.cpp 
     signal_database.cpp 
)

target_include_directories(vcm_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(vcm_core PUBLIC 
    Threads::Threads 
    nlohmann_json::nlohmann_json
    )

target_compile_options(vcm_core PRIVATE 
 -Wall -Wextra -Wpedantic )