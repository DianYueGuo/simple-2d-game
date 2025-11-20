cmake_minimum_required()

project(simple-2d-game)

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
    GIT_TAG v1.92.4
)
file(
    GLOB
    IMGUI_SOURCES
    "${imgui_SOURCE_DIR}/imgui*.cpp"
    "${imgui_SOURCE_DIR}/imgui*.h"
)
add_library(
    imgui
    STATIC
    ${IMGUI_SOURCES}
)

FetchContent_Declare(
    box2d
    GIT_REPOSITORY git@github.com:erincatto/box2d.git
    GIT_TAG v3.1.1
)
FetchContent_MakeAvailable(box2d)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(
    app
    src/main.cpp
    src/game.cpp
    src/cell.cpp
)
target_link_libraries(app PRIVATE SFML::Graphics SFML::Audio SFML::Network)
target_link_libraries(app PRIVATE imgui)
target_link_libraries(app PRIVATE box2d)
