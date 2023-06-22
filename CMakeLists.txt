# For fetch content
cmake_minimum_required(VERSION 3.0)

project(Planet)

# Link only when creating targets
add_executable(Planet main.cpp)
find_package(spdlog REQUIRED)
target_link_libraries(Planet PRIVATE spdlog::spdlog)

find_package(Vulkan REQUIRED)
target_link_libraries(Planet PRIVATE Vulkan::Vulkan)

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(Planet PRIVATE glfw)
