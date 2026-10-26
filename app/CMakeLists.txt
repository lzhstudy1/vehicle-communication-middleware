add_executable(vcm_app main.cpp)

target_link_libraries(vcm_app PRIVATE vcm_core)

target_compile_options(vcm_app PRIVATE 
     -Wall -Wextra -Wpedantic
     )