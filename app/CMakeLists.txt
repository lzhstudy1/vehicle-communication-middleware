add_executable(vcm_app main.cpp)

target_link_libraries(vcm_app PRIVATE vcm_core)

if(ENABLE_COVERAGE)
    target_compile_options(vcm_app PRIVATE --coverage -O0 -g)
    target_link_options(vcm_app PRIVATE --coverage)
endif()

target_compile_options(vcm_app PRIVATE 
     -Wall -Wextra -Wpedantic
     )