# CMakeList.txt: leanOpengl 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
 
file(GLOB SHADERS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp)

source_group("shaders" FILES ${SHADERS})


# 将源代码添加到此项目的可执行文件。
add_executable(${PROJECT_NAME}
    ${SHADERS}
)
