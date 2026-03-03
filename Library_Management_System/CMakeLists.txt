cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(LibraryManagementSystem VERSION 0.1)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add include directories
include_directories(include)

# Add the library sources
add_library(libLibraryManagementSystem
    lib/Application.cpp
    lib/Book.cpp
    lib/Database.cpp
    lib/Library.cpp
    lib/Menu.cpp
    lib/Multithreading.cpp
    lib/Networking.cpp
    lib/Persistence.cpp
    lib/Transaction.cpp
    lib/User.cpp
    lib/Utils.cpp
)

# Add the executable for the main program
add_executable(LibraryManagementSystem lib/main.cpp)

# Link the library to the executable
target_link_libraries(LibraryManagementSystem libLibraryManagementSystem)



# include(FetchContent)

# FetchContent_Declare(
#     json
#     URL https://github.com/nlohmann/json/releases/latest/download/json.tar.xz
# )

# FetchContent_MakeAvailable(json)

# target_link_libraries(libLibraryManagementSystem
#     PRIVATE
#         nlohmann_json::nlohmann_json
# )
