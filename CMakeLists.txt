cmake_minimum_required(VERSION 3.11)

project(Planet)

# Link only when creating targets
add_executable(Planet main.cpp)
find_package(spdlog REQUIRED)
target_link_libraries(Planet PRIVATE spdlog::spdlog)
