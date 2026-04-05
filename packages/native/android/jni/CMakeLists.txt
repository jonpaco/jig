cmake_minimum_required(VERSION 3.15)
project(jig_android C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Pull in jig_core (static library + libwebsockets + cJSON)
add_subdirectory(../../core ${CMAKE_BINARY_DIR}/core)

# Build the shared library that gets injected into APKs
add_library(jig SHARED JigStandaloneInit.c JigScreenshotShim.c)
target_link_libraries(jig PRIVATE jig_core)

# Note: pthread is part of Android's Bionic libc and does not need explicit linking

# Android system libraries
find_library(LOG_LIB log)
target_link_libraries(jig PRIVATE ${LOG_LIB})
