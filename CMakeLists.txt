# For fetch content
cmake_minimum_required(VERSION 3.0)

project(Planet)

# Set the stdlib to 20
set(CMAKE_CXX_STANDARD 20)

# Link only when creating targets
add_executable(Planet main.cpp)
find_package(spdlog REQUIRED)
target_link_libraries(Planet PRIVATE spdlog::spdlog)

find_package(Vulkan REQUIRED)
target_link_libraries(Planet PRIVATE Vulkan::Vulkan)

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(Planet PRIVATE glfw)

# /opt/homebrew/Cellar/glm/0.9.9.8/include/glm/
# target_include_directories(Planet PRIVATE /usr/local/include/glm)


# Needed to manually do this to get dynamic rendering to work
# https://github.com/KhronosGroup/MoltenVK/issues/1810
# Specify the path to your MoltenVK installation
# Replace with actual path
set(MOLTEN_VK_PATH "~/bar/MoltenVK")

# Find MoltenVK library
find_library(MOLTEN_VK_LIB MoltenVK PATHS ${MOLTEN_VK_PATH})

if(NOT MOLTEN_VK_LIB)
    message(FATAL_ERROR "MoltenVK library not found")
endif()

# Include MoltenVK headers
target_include_directories(Planet PRIVATE ${MOLTEN_VK_PATH}/include)

# Link MoltenVK library
target_link_libraries(Planet PRIVATE ${MOLTEN_VK_LIB})

add_custom_command(
  OUTPUT ../shaders/fullscreenquad.spv
  COMMAND glslangValidator -V ../shaders/fullscreenquad.vert -o ../shaders/fullscreenquad.spv
  DEPENDS ../shaders/fullscreenquad.vert
  COMMENT "Compiling fullscreenquad"
)

add_custom_target(
  fullscreenquad ALL
  DEPENDS ../shaders/fullscreenquad.spv
)

add_custom_command(
  OUTPUT ../shaders/planet.spv
  COMMAND glslangValidator -V ../shaders/planet.frag -o ../shaders/planet.spv
  DEPENDS ../shaders/planet.frag
  COMMENT "Compiling planet"
)

add_custom_target(
  plan ALL
  DEPENDS ../shaders/planet.spv
)


target_compile_options(${TARGET_NAME} Planet PRIVATE -Wno-c99-designator)

# if(MSVC)
#   target_compile_options(${TARGET_NAME} Planet PRIVATE /W4 /WX)
# else()
#   target_compile_options(${TARGET_NAME} Planet PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-c99-designator)
# endif()

