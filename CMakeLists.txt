cmake_minimum_required(VERSION 3.12)
project(TT_infotecs CXX)

set(CMAKE_CXX_STANDART 17)
set(CMAKE_CXX_STANDART_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(${CMAKE_SOURCE_DIR}/logging/include)

set(LOGGING_SOURCES
    logging/src/logger.cpp
    logging/src/file_sink.cpp
    logging/src/upd_sink.cpp
)

# static library
add_library(logging_static STATIC ${LOGGING_SOURCES})
set_target_properties(logging_static PROPERTIES OUTPUT_NAME logging)

# dinamic library
add_library(logging_shared SHARED ${LOGGING_SOURCES})
set_target_properties(logging_shared PROPERTIES OUTPUT_NAME logging)

# tests
enable_testing()

add_executable(t_logger_basic tests/test_logger_basic.cpp)
target_link_libraries(t_logger_basic PRIVATE logging_static pthread)
add_test(NAME logger_basic COMMAND t_logger_basic)
