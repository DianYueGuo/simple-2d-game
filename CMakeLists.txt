cmake_minimum_required()

project(simple-2d-game)

include(FetchContent)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
)
FetchContent_MakeAvailable(SFML)

add_executable(
    app
    src/main.cpp
)
target_link_libraries(app PRIVATE SFML::Graphics SFML::Audio SFML::Network)
