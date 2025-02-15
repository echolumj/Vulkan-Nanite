cmake_minimum_required (VERSION 3.13)
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)

file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_SOURCE_DIR}/src/*.h)
 
file(GLOB SHADERS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp)

source_group("shaders" FILES ${SHADERS})


# 将源代码添加到此项目的可执行文件。
add_executable(${PROJECT_NAME}
    ${HEADERS}
    ${SOURCES}
    ${SHADERS}
)

# load vulkan
find_package(Vulkan REQUIRED)
if(VULKAN_FOUND)
    target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARY})
    message(STATUS "Vulkan Version = ${Vulkan_VERSION}。")
else(VULKAN_FOUND)
    message(FATAL_ERROR ”VULKAN library not found”)
endif(VULKAN_FOUND)
