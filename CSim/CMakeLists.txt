cmake_minimum_required(VERSION 3.13)

project(CSim)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "/Wall /EHsc")
else ()
    set(CMAKE_CXX_FLAGS "-Wall")
endif ()
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


if (WIN32)
    add_executable(CSim  Source/Rulesets/BriansBrainRuleSet.cpp Source/Rulesets/CellRuleSet.cpp  Source/Rulesets/DayAndNightRuleSet.cpp  Source/Rulesets/GameOfLifeRuleSet.cpp  Source/Rulesets/HighlifeRuleSet.cpp  Source/Rulesets/LifeWithoutDeathRuleSet.cpp Source/Rulesets/Rule90RuleSet.cpp  Source/Rulesets/Rule184RuleSet.cpp  Source/Rulesets/SeedsRuleSet.cpp Source/Rulesets/WireworldRuleSet.cpp
            Source/Platform/Windows/WinMain.cpp
            Source/Init/SysInfo.cpp
            Source/AssetLoaders/TTFLoader.cpp
            Source/CellBrush.cpp
            Source/CellCanvas.cpp
            Source/CellCommandLine.cpp
            Source/CellEditState.cpp
            Source/CellLoadState.cpp
            Source/CellLogger.cpp
            Source/CellNormalState.cpp
            Source/CellPencilBrush.cpp
            Source/CellRoundBrush.cpp
            Source/CellSaveState.cpp
            Source/CellState.cpp
            Source/RenderFont.cpp
            Source/RenderWindow.cpp
            Source/SysCmdLine.cpp
            Source/CellMain.cpp
            Source/Platform/Windows/WinSaveLoad.cpp)
endif ()

if(UNIX)
    add_executable(CSim Source/Rulesets/BrainsBrainRuleSet.h Source/Rulesets/BriansBrainRuleSet.cpp Source/Rulesets/CellRuleSet.cpp Source/Rulesets/CellRuleSet.h Source/Rulesets/DayAndNightRuleSet.cpp Source/Rulesets/DayAndNightRuleSet.h Source/Rulesets/GameOfLifeRuleSet.cpp Source/Rulesets/GameOfLifeRuleSet.h Source/Rulesets/HighlifeRuleSet.cpp Source/Rulesets/HighlifeRuleSet.h Source/Rulesets/LifeWithoutDeathRuleSet.cpp Source/Rulesets/LifeWithoutDeathRuleSet.h Source/Rulesets/Rule90RuleSet.cpp Source/Rulesets/Rule90RuleSet.h Source/Rulesets/Rule184RuleSet.cpp Source/Rulesets/Rule184RuleSet.h Source/Rulesets/SeedsRuleSet.cpp Source/Rulesets/SeedsRuleSet.h Source/Rulesets/WireworldRuleSet.cpp Source/Rulesets/WireworldRuleSet.h
            Source/Platform/Linux/_main.cpp
            Source/Init/AllSets.h
            Source/Init/AllState.h
            Source/Init/BuildInfo.h
            Source/Init/MacroDefs.h
            Source/Init/SysInfo.cpp
            Source/Init/SysInfo.h
            Source/AssetLoaders/TTFLoader.cpp
            Source/AssetLoaders/TTFLoader.h
            Source/CellBrush.cpp
            Source/CellBrush.h
            Source/CellCanvas.cpp
            Source/CellCanvas.h
            Source/CellCommandLine.cpp
            Source/CellCommandLine.h
            Source/CellEditState.cpp
            Source/CellEditState.h
            Source/CellLoadState.cpp
            Source/CellLoadState.h
            Source/CellLogger.cpp
            Source/CellLogger.h
            Source/CellNormalState.cpp
            Source/CellNormalState.h
            Source/CellPencilBrush.cpp
            Source/CellPencilBrush.h
            Source/CellRoundBrush.cpp
            Source/CellRoundBrush.h
            Source/CellSaveState.cpp
            Source/CellSaveState.h
            Source/CellState.cpp
            Source/CellState.h
            Source/RenderContext.h
            Source/RenderFont.cpp
            Source/RenderWindow.cpp
            Source/RenderWindow.h
            Source/RenderFont.h
            Source/CellMain.cpp
            Source/CellMain.h
            Source/SaveLoad.h
            Source/SysCmdLine.h
            Source/SysCmdLine.cpp
            Source/Platform/Linux/LinuxSaveLoad.cpp
            Source/Init/MacroDefs.cpp
    )
endif ()

if(APPLE)

    message(FATAL_ERROR "Apple computers are not supported at this time!!")

endif ()


find_package(OpenGL REQUIRED)

#Include GLFW source
add_subdirectory("thirdparty/glfw-3.4")
add_subdirectory("thirdparty/glm")

#Add freetype
add_subdirectory("thirdparty/freetype-2.13.3")
add_subdirectory("thirdparty/glew-2.1.0/build/cmake")

include_directories("thirdparty/glfw-3.4/include")
include_directories("thirdparty/glew-2.1.0/include")

target_link_libraries(CSim freetype-interface)
target_link_libraries(CSim freetype)
target_link_libraries(CSim glew_s) # _s means static rather than dynamic
target_link_libraries(CSim glfw)
target_link_libraries(CSim OpenGL::GL)

#Copy over the shader files
file(COPY ${CMAKE_CURRENT_LIST_DIR}/Shader DESTINATION ${CMAKE_CURRENT_BINARY_DIR})