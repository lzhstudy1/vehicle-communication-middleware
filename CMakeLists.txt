cmake_minimum_required(VERSION 3.16)

project(vcm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Debug)
endif()

# 修复了拼写：BUILD_TESTS
option(BUILD_TESTS "build unit tests" ON)
option(GENERATE_JSON "Run Python script to generate vehicle.json" OFF)

find_package(Threads REQUIRED)

include(FetchContent)

# 修复了拼写：FetchContent_Declare 和 MakeAvailable
FetchContent_Declare(
   nlohmann_json
   GIT_REPOSITORY https://github.com/nlohmann/json.git
   GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json) # 你原来漏了这一行，不写这句库不会生效

if(BUILD_TESTS)
    FetchContent_Declare(
         googletest 
         GIT_REPOSITORY https://github.com/google/googletest.git
         GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

# 自动生成 json
if(GENERATE_JSON)
   # 修复：Python3 必须大写 P
   find_package(Python3 COMPONENTS Interpreter REQUIRED)
   add_custom_command(
      OUTPUT ${CMAKE_SOURCE_DIR}/dbc/vehicle.json
      COMMAND ${Python3_EXECUTABLE}
              ${CMAKE_SOURCE_DIR}/scripts/dbc_to_json.py
              ${CMAKE_SOURCE_DIR}/dbc/vehicle.dbc
              ${CMAKE_SOURCE_DIR}/dbc/vehicle.json
      DEPENDS
              ${CMAKE_SOURCE_DIR}/scripts/dbc_to_json.py
              ${CMAKE_SOURCE_DIR}/dbc/vehicle.dbc
      COMMENT "Generate vehicle.json from vehicle.dbc"
      VERBATIM
   )
    add_custom_target(generate_json ALL DEPENDS ${CMAKE_SOURCE_DIR}/dbc/vehicle.json)
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(tests)