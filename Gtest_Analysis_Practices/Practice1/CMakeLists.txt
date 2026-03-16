cmake_minimum_required(VERSION 3.13)

project(BankSystemTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────────────────────────────────────────────
# Fetch Google Test
# ─────────────────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.12.0)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BUILD_DIR})
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Create library from source files
# ─────────────────────────────────────────────────────────────────────────────
add_library(BankSystem src/BankSystem.cpp)
target_include_directories(BankSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ─────────────────────────────────────────────────────────────────────────────
# Create test executable
# ─────────────────────────────────────────────────────────────────────────────
add_executable(BankSystemTest test/BankSystemTest.cpp)
target_include_directories(BankSystemTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(BankSystemTest BankSystem gtest gmock gtest_main)

target_compile_options(BankSystemTest PRIVATE -fsanitize=address -g)
target_link_options(BankSystemTest PRIVATE -fsanitize=address)

# ─────────────────────────────────────────────────────────────────────────────
# Enable testing
# ─────────────────────────────────────────────────────────────────────────────
enable_testing()
add_test(NAME BankSystemTest COMMAND BankSystemTest)
