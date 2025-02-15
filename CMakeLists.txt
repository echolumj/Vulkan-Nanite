cmake_minimum_required (VERSION 3.13)
set(CMAKE_CXX_STANDARD 17) 
project(Vulkan-Nanite)

add_definitions(-DRELATIVE_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

# 包含子项目。
add_subdirectory ("src")
add_subdirectory ("third_party")