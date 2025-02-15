cmake_minimum_required (VERSION 3.13)
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)
set(GLAD_INCLUDE_DIR ${THIRD_PARTY_DIR}/glad)
set(GLFW_DIR ${THIRD_PARTY_DIR}/GLFW)

add_subdirectory(imgui)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${THIRD_PARTY_DIR}
)

target_sources(${PROJECT_NAME} PUBLIC ${GLAD_INCLUDE_DIR}/glad.c)

target_link_libraries(${PROJECT_NAME}
    ${GLFW_DIR}/glfw3.lib
)

