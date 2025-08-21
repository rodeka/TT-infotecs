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

# приложения для логирования (часть 2)
add_executable(log_app apps/log_app/main.cpp)
target_link_libraries(log_app PRIVATE logging_static pthread)

# приложение для сбора статистики (часть 3).
add_executable(log_stats apps/log_stats/main.cpp)
target_link_libraries(log_stats PRIVATE logging_static pthread)

# tests
enable_testing()

add_executable(t_logger_basic tests/test_logger_basic.cpp)
target_link_libraries(t_logger_basic PRIVATE logging_static pthread)
add_test(NAME logger_basic COMMAND t_logger_basic)

# udp sink петля
add_executable(t_udp_sink tests/test_udp_sink.cpp)
target_link_libraries(t_udp_sink PRIVATE logging_static pthread)
add_test(NAME udp_sink COMMAND t_udp_sink)

# ошибки инициализации
add_executable(t_init_errors tests/test_init_errors.cpp)
target_link_libraries(t_init_errors PRIVATE logging_static pthread)
add_test(NAME init_errors COMMAND t_init_errors)

# формат времени
add_executable(t_timefmt tests/test_timefmt.cpp)
target_link_libraries(t_timefmt PRIVATE logging_static pthread)
add_test(NAME timefmt COMMAND t_timefmt)