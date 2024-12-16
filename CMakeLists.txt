cmake_minimum_required(VERSION 3.10)

# Project name and language
project(Multithreaded-Proxy-Server)

# Set C++ standard to C++20 (you can adjust this to a lower version if needed)
set(CMAKE_CXX_STANDARD 20)

# Specify the paths to Prometheus library and include directories

# the source files for your project
add_executable(server main.cpp ThreadPool.cpp Lrucache.cpp Server.cpp Logger.cpp)

# External libraries (pthread, spdlog, fmt)
target_link_libraries(server pthread spdlog fmt)

#  C++20 as the required standard 
target_compile_options(server PRIVATE -std=c++20)
