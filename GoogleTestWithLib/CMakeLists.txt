cmake_minimum_required(VERSION 3.13)
project(GoogleTestWithLib)
set(CMAKE_CXX_STANDARD 11)

find_package(GTest REQUIRED)
message("GTest_LIBRARIES = ${GTEST_LIBRARIES}")

add_library(commonLibrary LibraryCode.cpp)

add_executable(mainApp main.cpp)
target_link_libraries(mainApp commonLibrary)

add_executable(unitTestRunner testRunner.cpp)
target_link_libraries(unitTestRunner commonLibrary ${GTEST_LIBRARIES} pthread)

