cmake_minimum_required(VERSION 3.15)
project(jig_core C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- libwebsockets via FetchContent ---
include(FetchContent)
FetchContent_Declare(
    libwebsockets
    GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
    GIT_TAG v4.3-stable
    GIT_SHALLOW TRUE
)
# Strip to absolute minimum -- no SSL, no test apps, no extras
set(LWS_WITH_SSL OFF CACHE BOOL "" FORCE)
set(LWS_WITH_SHARED OFF CACHE BOOL "" FORCE)
set(LWS_WITH_STATIC ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TESTAPPS ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TEST_SERVER ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TEST_PING ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TEST_CLIENT ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_EXTENSIONS ON CACHE BOOL "" FORCE)
set(LWS_WITH_MINIMAL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LWS_WITH_HTTP2 OFF CACHE BOOL "" FORCE)
set(LWS_WITH_ZIP_FOPS OFF CACHE BOOL "" FORCE)
set(LWS_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(LWS_WITH_RANGES OFF CACHE BOOL "" FORCE)
set(LWS_WITH_ACCESS_LOG OFF CACHE BOOL "" FORCE)
set(LWS_IPV6 OFF CACHE BOOL "" FORCE)
set(LWS_ROLE_H1 ON CACHE BOOL "" FORCE)
set(LWS_ROLE_WS ON CACHE BOOL "" FORCE)
set(LWS_ROLE_H2 OFF CACHE BOOL "" FORCE)
set(LWS_ROLE_MQTT OFF CACHE BOOL "" FORCE)
set(LWS_WITH_DIR OFF CACHE BOOL "" FORCE)
set(LWS_WITH_LEJP OFF CACHE BOOL "" FORCE)
set(LWS_WITH_LWSAC OFF CACHE BOOL "" FORCE)
set(LWS_WITH_SEQUENCER OFF CACHE BOOL "" FORCE)
set(LWS_WITHOUT_BUILTIN_GETIFADDRS ON CACHE BOOL "" FORCE)
set(LWS_WERROR OFF CACHE BOOL "" FORCE)
set(DISABLE_WERROR ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(libwebsockets)

# Ensure lws doesn't fail on compiler warnings (some versions have harmless ones)
if(TARGET websockets)
    get_target_property(_lws_opts websockets COMPILE_OPTIONS)
    if(_lws_opts)
        list(REMOVE_ITEM _lws_opts "-Werror")
        set_target_properties(websockets PROPERTIES COMPILE_OPTIONS "${_lws_opts}")
    endif()
endif()

# --- cJSON vendor library ---
add_library(cjson STATIC vendor/cJSON/cJSON.c)
target_include_directories(cjson PUBLIC vendor/cJSON)

# --- jig_core static library ---
add_library(jig_core STATIC
    jig_platform.c
    jig_app_info.c
    jig_errors.c
    jig_session.c
    jig_dispatcher.c
    jig_server.c
    handlers/jig_handshake.c
    middleware/jig_handshake_gate.c
    middleware/jig_domain_guard.c
)
target_include_directories(jig_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(jig_core PUBLIC ${libwebsockets_BINARY_DIR}/include)
target_link_libraries(jig_core PUBLIC cjson websockets)

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

add_executable(test_platform tests/test_platform.c)
target_link_libraries(test_platform jig_core)
add_test(NAME test_platform COMMAND test_platform)

add_executable(test_dispatcher tests/test_dispatcher.c)
target_link_libraries(test_dispatcher jig_core)
add_test(NAME test_dispatcher COMMAND test_dispatcher)

add_executable(test_handshake tests/test_handshake.c)
target_link_libraries(test_handshake jig_core)
add_test(NAME test_handshake COMMAND test_handshake)

add_executable(test_middleware tests/test_middleware.c)
target_link_libraries(test_middleware jig_core)
add_test(NAME test_middleware COMMAND test_middleware)

add_executable(test_server tests/test_server.c)
target_link_libraries(test_server jig_core)
add_test(NAME test_server COMMAND test_server)
