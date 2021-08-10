cmake_minimum_required(VERSION 3.0.0)
project(Procedural-Planet VERSION 0.1.0)

############# options ##################
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
add_subdirectory(deps/glfw)

################## headers files ###################
include_directories(deps/glad/include/
                    deps/glfw/include/
                    deps/glm/
                    deps/stb/
                    deps/imgui/
                    src/headers)

file(GLOB GLAD_SRC deps/glad/src/glad.c)

file(GLOB IMGUI_SRC deps/imgui/*.cpp)

file(GLOB_RECURSE PROJ_HEADERS src/*.h
                               src/*.hpp)

file(GLOB_RECURSE PROJ_SOURCES src/*.c
                               src/*.cpp)

file(GLOB_RECURSE PROJ_SHADERS res/shaders/*.frag
                               res/shaders/*.vert)

source_group("glad" FILES ${GLAD_SRC})
source_group("imgui" FILES ${IMGUI_SRC})
source_group("shaders" FILES ${PROJ_SHADERS})
source_group("src" FILES ${PROJ_SOURCES})
source_group("headers" FILES ${PROJ_HEADERS})

add_definitions(-DPROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\")

################## linker ###################
add_executable(${PROJECT_NAME} ${PROJ_SOURCES} ${PROJ_HEADERS} ${PROJ_SHADERS} ${GLAD_SRC} ${IMGUI_SRC})

target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} ${GLAD_LIBRARIES})

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${PROJECT_NAME}>/data
    DEPENDS ${PROJ_SHADERS}
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
