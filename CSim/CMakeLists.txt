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

# Shared application sources (platform entry points added per-OS below)
set(CSIM_COMMON_SOURCES
    Source/Rulesets/BriansBrainRuleSet.cpp
    Source/Rulesets/RuleSet.cpp
    Source/Rulesets/DayAndNightRuleSet.cpp
    Source/Rulesets/GameOfLifeRuleSet.cpp
    Source/Rulesets/HighlifeRuleSet.cpp
    Source/Rulesets/LifeWithoutDeathRuleSet.cpp
    Source/Rulesets/Rule90RuleSet.cpp
    Source/Rulesets/Rule184RuleSet.cpp
    Source/Rulesets/SeedsRuleSet.cpp
    Source/Rulesets/WireworldRuleSet.cpp
    Source/Foundation/SysInfo.cpp
    Source/Assets/TTFLoader.cpp
    Source/Game/Canvas.cpp
    Source/Game/CellGameModule.cpp
    Source/Services/CommandLine.cpp
    Source/Services/Logger.cpp
    Source/Services/SysCmdLine.cpp
    Source/Services/EnvVars.cpp
    Source/Services/DebugAlloc.cpp
    Source/App/CellMain.cpp
    Source/Engine/Illumo.cpp
    Source/Engine/DebugModule.cpp
    Source/Rendering/RenderWindow.cpp
    Source/Rendering/GLString.cpp
    Source/Rendering/Camera.cpp
    Source/Rendering/SplashText.cpp
    Source/Rendering/OpenGL/GLBackend.cpp
    Source/Rendering/OpenGL/GLBuffer.cpp
    Source/Rendering/OpenGL/GLDevice.cpp
    thirdparty/tracy-0.13.1/public/TracyClient.cpp
)

# --- Define Executable based on OS ---
if (WIN32)
    add_executable(CSim
        ${CSIM_COMMON_SOURCES}
        Source/Platform/Windows/WinMain.cpp
        Source/Platform/Windows/WinSaveLoad.cpp
    )
endif ()

if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK REQUIRED gtkmm-3.0)

    add_definitions(${GTK_CFLAGS_OTHER})

    add_executable(CSim
        ${CSIM_COMMON_SOURCES}
        Source/Platform/Linux/_main.cpp
        Source/Platform/Linux/LinuxSaveLoad.cpp
    )

    target_link_libraries(CSim PRIVATE ${GTK_LIBRARIES})
    target_include_directories(CSim SYSTEM PRIVATE ${GTK_INCLUDE_DIRS})
    target_include_directories(CSim PRIVATE thirdparty/tracy-0.13.1)
    target_link_directories(CSim PRIVATE ${GTK_LIBRARY_DIRS})
endif ()

if(APPLE)
    find_package(OpenGL REQUIRED)

    add_executable(CSim
        ${CSIM_COMMON_SOURCES}
        Source/Platform/macOS/Main.cpp
        Source/Platform/macOS/MacSaveLoad.mm
    )

    target_link_libraries(CSim PRIVATE "-framework Cocoa")
    target_link_libraries(CSim PRIVATE "-framework OpenGL")
    target_link_libraries(CSim PRIVATE "-framework IOKit")
    target_link_libraries(CSim PRIVATE "-framework CoreVideo")
endif()

# Package include roots (prefer Source/<Package>/... in includes when possible)
target_include_directories(CSim PRIVATE
    "Source"
    "Source/App"
    "Source/Engine"
    "Source/Game"
    "Source/Services"
    "Source/Foundation"
    "Source/Assets"
    "Source/Rendering"
    "Source/Rulesets"
    "Source/Platform"
    "."
    "thirdparty/tracy-0.13.1/public"
)

target_include_directories(CSim SYSTEM PRIVATE
    "thirdparty/glfw-3.4/include"
    "thirdparty/glew-2.1.0/include"
    "thirdparty/json/single_include"
    "thirdparty"
)

target_link_libraries(CSim PRIVATE
    freetype-interface
    freetype
    glew_s
    glfw
    OpenGL::GL
    glm::glm
)

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

# ---------------------------------------------------------------------------
# Phase 6: headless MockBackend tests (no OpenGL / no window)
# ---------------------------------------------------------------------------
enable_testing()

add_executable(CSimRenderTests
	Source/Tests/TestMockBackend.cpp
	Source/Tests/TestRendererE2E.cpp
	# Production pieces needed for e2e (headless; no WinMain / no GLBackend objects if inject-only,
	# but Renderer.h includes GLBackend types so we still link GLEW/GLFW for headers).
	Source/Game/Canvas.cpp
	Source/Rendering/Camera.cpp
	Source/Services/EnvVars.cpp
	Source/Services/Logger.cpp
	# Logger forwards to CommandLine; only need object symbols (no console UI in tests).
	Source/Services/CommandLine.cpp
)

target_include_directories(CSimRenderTests PRIVATE
	"Source"
	"Source/App"
	"Source/Engine"
	"Source/Game"
	"Source/Services"
	"Source/Foundation"
	"Source/Assets"
	"Source/Rendering"
	"Source/Rulesets"
	"Source/Platform"
	"."
	"thirdparty/tracy-0.13.1/public"
)

target_include_directories(CSimRenderTests SYSTEM PRIVATE
	"thirdparty/glfw-3.4/include"
	"thirdparty/glew-2.1.0/include"
	"thirdparty/json/single_include"
	"thirdparty"
)

target_compile_features(CSimRenderTests PRIVATE cxx_std_23)

target_link_libraries(CSimRenderTests PRIVATE
	glew_s
	glfw
	OpenGL::GL
	glm::glm
)

if(MSVC)
	target_compile_options(CSimRenderTests PRIVATE /W4 /FS)
else()
	target_compile_options(CSimRenderTests PRIVATE -Wall -Wextra)
endif()

add_test(NAME MockBackend COMMAND CSimRenderTests)
set_tests_properties(MockBackend PROPERTIES
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
