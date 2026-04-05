cmake_minimum_required(VERSION 3.15)
project(jig_core C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- cJSON vendor library ---
add_library(cjson STATIC vendor/cJSON/cJSON.c)
target_include_directories(cjson PUBLIC vendor/cJSON)

# --- jig_core static library ---
add_library(jig_core STATIC
    jig_platform.c
    jig_app_info.c
    jig_errors.c
    jig_session.c
)
target_include_directories(jig_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(jig_core PUBLIC cjson)

# --- Tests ---
enable_testing()

add_executable(test_errors tests/test_errors.c)
target_link_libraries(test_errors jig_core)
add_test(NAME test_errors COMMAND test_errors)

add_executable(test_session tests/test_session.c)
target_link_libraries(test_session jig_core)
add_test(NAME test_session COMMAND test_session)

add_executable(test_app_info tests/test_app_info.c)
target_link_libraries(test_app_info jig_core)
add_test(NAME test_app_info COMMAND test_app_info)
