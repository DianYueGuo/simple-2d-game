cmake_minimum_required(VERSION 3.11)

project(simple-2d-game)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(
        CMAKE_BUILD_TYPE
        Debug
        CACHE STRING "Choose the type of build." FORCE
    )
    set_property(
        CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel"
    )
endif()

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
)
FetchContent_MakeAvailable(SFML)

FetchContent_Populate(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.1
)
file(
    GLOB
    IMGUI_SOURCES
    CONFIGURE_DEPENDS
    "${imgui_SOURCE_DIR}/imgui*.cpp"
    "${imgui_SOURCE_DIR}/imgui*.h"
)
add_library(
    imgui
    STATIC
    ${IMGUI_SOURCES}
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

FetchContent_Declare(
    imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v3.0
)
set(IMGUI_SFML_FIND_SFML OFF)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
FetchContent_MakeAvailable(imgui-sfml)

FetchContent_Declare(
    box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v3.1.1
)
FetchContent_MakeAvailable(box2d)

set(MULTINEAT_ENABLE_PYTHON OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    NEAT
    GIT_REPOSITORY https://github.com/DianYueGuo/NEAT.git
    GIT_TAG fix-cmake-version
)
FetchContent_MakeAvailable(NEAT)

add_executable(
    app
    src/main.cpp
    src/game.cpp
    src/circle_physics.cpp
    src/drawable_circle.cpp
    src/eatable_circle.cpp
    src/eater_circle.cpp
    src/eater_brain.cpp
    src/ui.cpp
)
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(app PRIVATE SFML::Graphics SFML::Audio SFML::Network)
target_link_libraries(app PRIVATE imgui)
target_link_libraries(app PRIVATE ImGui-SFML::ImGui-SFML)
target_link_libraries(app PRIVATE box2d)
target_link_libraries(app PRIVATE neat)

option(ENABLE_WARNINGS "Enable compiler warnings" ON)
option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

if(ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(app PRIVATE /W4)
        if(ENABLE_WARNINGS_AS_ERRORS)
            target_compile_options(app PRIVATE /WX)
        endif()
    else()
        target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic)
        if(ENABLE_WARNINGS_AS_ERRORS)
            target_compile_options(app PRIVATE -Werror)
        endif()
    endif()
endif()
