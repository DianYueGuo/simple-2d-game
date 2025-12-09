cmake_minimum_required(VERSION 3.11)

project(petri-dish-simulation VERSION 0.1.0)

set(APP_TARGET "petridish")
set(APP_BUNDLE_NAME "Petri Dish Simulation")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
endif()

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
    ${APP_TARGET}
    MACOSX_BUNDLE
    src/main.cpp
    src/game.cpp
    src/circle_physics.cpp
    src/drawable_circle.cpp
    src/eatable_circle.cpp
    src/creature_circle.cpp
    src/ui.cpp
    src/game/spawner.cpp
    src/game/selection_manager.cpp
)
target_include_directories(${APP_TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(${APP_TARGET} PRIVATE SFML::Graphics SFML::Audio SFML::Network)
target_link_libraries(${APP_TARGET} PRIVATE imgui)
target_link_libraries(${APP_TARGET} PRIVATE ImGui-SFML::ImGui-SFML)
target_link_libraries(${APP_TARGET} PRIVATE box2d)
target_link_libraries(${APP_TARGET} PRIVATE neat)

option(ENABLE_WARNINGS "Enable compiler warnings" ON)
option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

if(ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(${APP_TARGET} PRIVATE /W4)
        if(ENABLE_WARNINGS_AS_ERRORS)
            target_compile_options(${APP_TARGET} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${APP_TARGET} PRIVATE -Wall -Wextra -Wpedantic)
        if(ENABLE_WARNINGS_AS_ERRORS)
        target_compile_options(${APP_TARGET} PRIVATE -Werror)
        endif()
    endif()
endif()

set_target_properties(
    ${APP_TARGET}
    PROPERTIES
        OUTPUT_NAME ${APP_BUNDLE_NAME}
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME ${APP_BUNDLE_NAME}
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.petridishsimulation"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
)

add_custom_target(
    run
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}
            ${CMAKE_COMMAND} -E env
            "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:${APP_TARGET}>"
            "$<TARGET_FILE:${APP_TARGET}>"
    DEPENDS ${APP_TARGET}
    USES_TERMINAL
    COMMENT "Build and run ${APP_BUNDLE_NAME}"
)

install(
    TARGETS ${APP_TARGET}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)

if(APPLE)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FixupBundle.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/FixupBundle.cmake @ONLY)
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/FixupBundle.cmake")
endif()
