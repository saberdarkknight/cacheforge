cmake_minimum_required(VERSION 3.25)

project(ForgeKV VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(forgekv
  src/db.cc
  src/forgekv.cc
  src/memtable.cc
  src/status.cc
  src/wal.cc
)

target_include_directories(forgekv
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(forgekv PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(forgekv PRIVATE /W4 /permissive-)
else()
  target_compile_options(forgekv PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if(BUILD_TESTING)
  add_executable(forgekv_hello_test
    tests/hello_test.cc
  )

  target_link_libraries(forgekv_hello_test PRIVATE forgekv)

  add_test(NAME forgekv_hello_test COMMAND forgekv_hello_test)

  add_executable(forgekv_status_test
    tests/status_test.cc
  )

  target_link_libraries(forgekv_status_test PRIVATE forgekv)

  add_test(NAME forgekv_status_test COMMAND forgekv_status_test)

  add_executable(forgekv_db_test
    tests/db_test.cc
  )

  target_link_libraries(forgekv_db_test PRIVATE forgekv)

  add_test(NAME forgekv_db_test COMMAND forgekv_db_test)
endif()
