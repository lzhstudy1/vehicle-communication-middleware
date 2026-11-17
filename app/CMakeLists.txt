add_executable(vcm_app main.cpp)
add_executable(vcm_client vcm_client.cpp)

target_link_libraries(vcm_app PRIVATE vcm_core)
target_link_libraries(vcm_client PRIVATE vcm_core)

if(ENABLE_COVERAGE)
    target_compile_options(vcm_app PRIVATE --coverage -O0 -g)
    target_link_options(vcm_app PRIVATE --coverage)
    target_compile_options(vcm_client PRIVATE --coverage -O0 -g)
    target_link_options(vcm_client PRIVATE --coverage)
endif()

target_compile_options(vcm_app PRIVATE 
     -Wall -Wextra -Wpedantic
     )
     target_compile_options(vcm_client PRIVATE 
     -Wall -Wextra -Wpedantic
     )