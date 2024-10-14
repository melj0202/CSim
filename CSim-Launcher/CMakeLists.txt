cmake_minimum_required(VERSION 3.10)

project(CSim-Launcher)

#Dependency setup

#wxWidgets

add_subdirectory("ThirdParty/wxWidgets")

if (WIN32)
add_executable(CSim-Launcher WIN32
        Source/cApp.cpp
        )
endif()
if (UNIX)
    add_executable(CSim-Launcher
            Source/cApp.cpp
    )
endif()
target_link_libraries(CSim-Launcher wx::net wx::core wx::base)