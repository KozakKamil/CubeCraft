cmake_minimum_required(VERSION 3.20)
project (CubeCraft VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find libs installed by vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)

# Executable
add_executable(CubeCraft
    src/main.cpp
)

# Link libraries
target_link_libraries(CubeCraft PRIVATE glfw glad::glad glm::glm)

target_include_directories(CubeCraft PRIVATE ${Stb_INCLUDE_DIR})

add_custom_command(TARGET CubeCraft POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:CubeCraft>/shaders
)

# Set starting project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CubeCraft)