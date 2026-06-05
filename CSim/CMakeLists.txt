cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED CSIM_DIR)
    set(CSIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

project(CSim)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

# Include Third-Party Sources
add_subdirectory("thirdparty/glfw-3.4" SYSTEM)
add_subdirectory("thirdparty/glm" SYSTEM)
add_subdirectory("thirdparty/freetype-2.13.3" SYSTEM)
add_subdirectory("thirdparty/glew-2.1.0/build/cmake" SYSTEM)

add_subdirectory("Source/")

# --- Define Executable based on OS ---
if (WIN32)
    add_executable(CSim  
        Source/Rulesets/BriansBrainRuleSet.cpp Source/Rulesets/RuleSet.cpp  Source/Rulesets/DayAndNightRuleSet.cpp  Source/Rulesets/GameOfLifeRuleSet.cpp  Source/Rulesets/HighlifeRuleSet.cpp  Source/Rulesets/LifeWithoutDeathRuleSet.cpp Source/Rulesets/Rule90RuleSet.cpp  Source/Rulesets/Rule184RuleSet.cpp  Source/Rulesets/SeedsRuleSet.cpp Source/Rulesets/WireworldRuleSet.cpp
        Source/Platform/Windows/WinMain.cpp
        Source/Init/SysInfo.cpp
        Source/AssetLoaders/TTFLoader.cpp
        Source/Core/Canvas.cpp
        Source/System/CommandLine.cpp
        Source/System/Logger.cpp
        Source/Rendering/RenderWindow.cpp
        Source/Rendering/GLString.cpp
        Source/Rendering/Drawable.cpp
        Source/Rendering/Camera.cpp
        Source/System/SysCmdLine.cpp
        Source/System/CellMain.cpp
        Source/Platform/Windows/WinSaveLoad.cpp
        Source/System/EnvVars.cpp
        Source/Rendering/RenderQueue.cpp
        Source/Rendering/SplashText.cpp
        Source/Rendering/OpenGL/GLBackend.cpp
        Source/Rendering/OpenGL/GLBuffer.cpp
        Source/Rendering/OpenGL/GLDevice.cpp
        Source/Core/CellGameModule.cpp
        Source/System/Illumo.cpp
        Source/System/DebugModule.cpp
        Source/System/DebugAlloc.cpp
        thirdparty/tracy-0.13.1/public/TracyClient.cpp
    )
endif ()

if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK REQUIRED gtkmm-3.0)

    # Add other flags to the compiler
    add_definitions(${GTK_CFLAGS_OTHER})

    add_executable(CSim 
        Source/Rulesets/BriansBrainRuleSet.cpp Source/Rulesets/RuleSet.cpp Source/Rulesets/DayAndNightRuleSet.cpp Source/Rulesets/GameOfLifeRuleSet.cpp Source/Rulesets/HighlifeRuleSet.cpp Source/Rulesets/LifeWithoutDeathRuleSet.cpp Source/Rulesets/Rule90RuleSet.cpp Source/Rulesets/Rule184RuleSet.cpp Source/Rulesets/SeedsRuleSet.cpp Source/Rulesets/WireworldRuleSet.cpp
        Source/Platform/Linux/_main.cpp
        Source/Init/SysInfo.cpp
        Source/AssetLoaders/TTFLoader.cpp
        Source/Core/Brush.cpp
        Source/Core/Canvas.cpp
        Source/System/CommandLine.cpp
        Source/States/EditState.cpp
        Source/States/LoadState.cpp
        Source/System/Logger.cpp
        Source/States/NormalState.cpp
        Source/Core/PencilBrush.cpp
        Source/Core/RoundBrush.cpp
        Source/States/SaveState.cpp
        Source/States/State.cpp
        Source/Rendering/RenderFont.cpp
        Source/Rendering/RenderWindow.cpp
        Source/Rendering/Drawable.cpp
        Source/Rendering/RenderQueue.cpp
        Source/System/CellMain.cpp
        Source/System/SysCmdLine.cpp
        Source/Platform/Linux/LinuxSaveLoad.cpp
        Source/System/EnvVars.cpp
        Source/System/DebugAlloc.cpp
        thirdparty/tracy-0.13.1/public/TracyClient.cpp
    )
    
    # FIXED: Added PRIVATE keyword here to fix signature error
    target_link_libraries(CSim PRIVATE ${GTK_LIBRARIES})
    target_include_directories(CSim SYSTEM PRIVATE ${GTK_INCLUDE_DIRS})
    target_include_directories(CSim PRIVATE
    thirdparty/tracy-0.13.1
)
    target_link_directories(CSim PRIVATE ${GTK_LIBRARY_DIRS})
endif ()

if(APPLE)
    find_package(OpenGL REQUIRED)
    
    add_executable(CSim  
        Source/Rulesets/BriansBrainRuleSet.cpp Source/Rulesets/RuleSet.cpp Source/Rulesets/DayAndNightRuleSet.cpp Source/Rulesets/GameOfLifeRuleSet.cpp Source/Rulesets/HighlifeRuleSet.cpp Source/Rulesets/LifeWithoutDeathRuleSet.cpp Source/Rulesets/Rule90RuleSet.cpp Source/Rulesets/Rule184RuleSet.cpp Source/Rulesets/SeedsRuleSet.cpp Source/Rulesets/WireworldRuleSet.cpp
        Source/Platform/macOS/Main.cpp
        Source/Platform/macOS/MacSaveLoad.mm
        Source/Init/SysInfo.cpp
        Source/AssetLoaders/TTFLoader.cpp
        Source/Core/Brush.cpp
        Source/Core/Canvas.cpp
        Source/System/CommandLine.cpp
        Source/States/EditState.cpp
        Source/States/LoadState.cpp
        Source/System/Logger.cpp
        Source/States/NormalState.cpp
        Source/Core/PencilBrush.cpp
        Source/Core/RoundBrush.cpp
        Source/States/SaveState.cpp
        Source/States/State.cpp
        Source/Rendering/RenderWindow.cpp
        Source/Rendering/GLString.cpp
        Source/Rendering/Drawable.cpp
        Source/System/SysCmdLine.cpp
        Source/System/CellMain.cpp
        Source/System/EnvVars.cpp
        Source/System/DebugAlloc.cpp
        thirdparty/tracy-0.13.1/public/TracyClient.cpp
    )
    
    target_link_libraries(CSim PRIVATE "-framework Cocoa")
    target_link_libraries(CSim PRIVATE "-framework OpenGL")
    target_link_libraries(CSim PRIVATE "-framework IOKit")
    target_link_libraries(CSim PRIVATE "-framework CoreVideo")
endif()

# --- Target Header Include Management ---
# Using modern target_include_directories instead of global include_directories
target_include_directories(CSim PRIVATE 
    "Source" "Source/Core" "Source/States" "Source/Rendering" 
    "Source/System" "Source/Rulesets" "Source/Init" "Source/AssetLoaders" "."
    "thirdparty/tracy-0.13.1/public"
)

# FIXED: Forcing all Third-Party directories to be cleanly registered as SYSTEM headers 
target_include_directories(CSim SYSTEM PRIVATE 
    "thirdparty/glfw-3.4/include"
    "thirdparty/glew-2.1.0/include"
    "thirdparty/json/single_include"
    "thirdparty"
)

# --- Target Linking ---
# FIXED: Removed the invalid "SYSTEM" library names from link execution blocks
target_link_libraries(CSim PRIVATE 
    freetype-interface
    freetype
    glew_s 
    glfw
    OpenGL::GL
    glm::glm
)

# --- Platform Compiler Flags Configuration ---
# FIXED: Cleaned up MSVC flags to eliminate overlapping /EHs entries (Warning D9025)
if(MSVC)

    target_compile_options(CSim PRIVATE
        /W4
        /FS

        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/fsanitize=address>

        $<$<CONFIG:Release>:/O2>
    )

else()

    target_compile_options(CSim PRIVATE
        -Wall
        -Wextra

        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-fsanitize=address>

        $<$<CONFIG:Release>:-O3>
    )

endif()


target_compile_definitions(CSim PRIVATE
    $<$<CONFIG:Debug>:TRACY_ENABLE>
)


# Copy over the shader files to build execution paths
file(COPY ${CMAKE_CURRENT_LIST_DIR}/Shader DESTINATION ${CMAKE_CURRENT_BINARY_DIR})