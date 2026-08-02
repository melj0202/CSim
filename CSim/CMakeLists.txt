cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED CSIM_DIR)
    set(CSIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

project(CSim)

find_package(OpenGL REQUIRED)

# Include Third-Party Sources
add_subdirectory("thirdparty/glfw-3.4" SYSTEM)
add_subdirectory("thirdparty/glm" SYSTEM)
add_subdirectory("thirdparty/freetype-2.13.3" SYSTEM)
add_subdirectory("thirdparty/glew-2.1.0/build/cmake" SYSTEM)

# Sources compiled by both the application and the headless test executable.
# Keep this as a source list instead of a static library: CSim enables Tracy in
# Debug while CSimTests intentionally compiles these files without Tracy.
set(CSIM_SHARED_SOURCES
    Source/Rulesets/BriansBrainRuleSet.cpp
    Source/Rulesets/RuleSet.cpp
    Source/Rulesets/DayAndNightRuleSet.cpp
    Source/Rulesets/GameOfLifeRuleSet.cpp
    Source/Rulesets/HighlifeRuleSet.cpp
    Source/Rulesets/LifeWithoutDeathRuleSet.cpp
    Source/Rulesets/SeedsRuleSet.cpp
    Source/Rulesets/WireworldRuleSet.cpp
    Source/Game/Canvas.cpp
    Source/Services/CommandLine.cpp
    Source/Services/Logger.cpp
    Source/Services/EnvVars.cpp
    Source/Rendering/GLString.cpp
    Source/Rendering/Camera.cpp
)

set(CSIM_APP_SOURCES
    ${CSIM_SHARED_SOURCES}
    Source/Rulesets/Rule90RuleSet.cpp
    Source/Rulesets/Rule184RuleSet.cpp
    Source/Foundation/SysInfo.cpp
    Source/Assets/TTFLoader.cpp
    Source/Game/CellGameModule.cpp
    Source/Services/SysCmdLine.cpp
    Source/Services/DebugAlloc.cpp
    Source/App/CellMain.cpp
    Source/Engine/Illumo.cpp
    Source/Engine/DebugModule.cpp
    Source/Rendering/RenderWindow.cpp
    Source/Rendering/SplashText.cpp
    Source/Rendering/OpenGL/GLBackend.cpp
    Source/Rendering/OpenGL/GLBuffer.cpp
    Source/Rendering/OpenGL/GLDevice.cpp
    thirdparty/tracy-0.13.1/public/TracyClient.cpp
)

set(CSIM_TEST_SOURCES
    Source/Tests/TestMain.cpp
    Source/Tests/TestMockBackend.cpp
    Source/Tests/TestRendererE2E.cpp
    Source/Tests/TestRuleSets.cpp
    Source/Tests/TestCellContext.cpp
    Source/Tests/TestCanvasDomain.cpp
    Source/Tests/TestUITokens.cpp
)

# Settings that truly are common to production and test targets. Application-
# only profiling, optimization, and sanitizer flags remain on CSim below.
function(csim_configure_target target_name)
    target_compile_features(${target_name} PRIVATE cxx_std_23)

    target_include_directories(${target_name} PRIVATE
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

    target_include_directories(${target_name} SYSTEM PRIVATE
        "thirdparty/glfw-3.4/include"
        "thirdparty/glew-2.1.0/include"
        "thirdparty/json/single_include"
        "thirdparty"
    )

    target_link_libraries(${target_name} PRIVATE
        glew_s
        glfw
        OpenGL::GL
        glm::glm
    )

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /FS)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra)
    endif()
endfunction()

# Select only the platform entry points here; the executable is defined once.
set(CSIM_PLATFORM_SOURCES)
set(CSIM_PLATFORM_LIBRARIES)

if(WIN32)
    list(APPEND CSIM_PLATFORM_SOURCES
        Source/Platform/Windows/WinMain.cpp
        Source/Platform/Windows/WinSaveLoad.cpp
    )
elseif(APPLE)
    list(APPEND CSIM_PLATFORM_SOURCES
        Source/Platform/macOS/Main.cpp
        Source/Platform/macOS/MacSaveLoad.mm
    )
    list(APPEND CSIM_PLATFORM_LIBRARIES
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK REQUIRED gtkmm-3.0)
    list(APPEND CSIM_PLATFORM_SOURCES
        Source/Platform/Linux/_main.cpp
        Source/Platform/Linux/LinuxSaveLoad.cpp
    )
    list(APPEND CSIM_PLATFORM_LIBRARIES ${GTK_LIBRARIES})
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

add_executable(CSim
    ${CSIM_APP_SOURCES}
    ${CSIM_PLATFORM_SOURCES}
)
csim_configure_target(CSim)

target_link_libraries(CSim PRIVATE
    freetype-interface
    freetype
    ${CSIM_PLATFORM_LIBRARIES}
)

if(UNIX AND NOT APPLE)
    target_compile_options(CSim PRIVATE ${GTK_CFLAGS_OTHER})
    target_include_directories(CSim SYSTEM PRIVATE ${GTK_INCLUDE_DIRS})
    target_link_directories(CSim PRIVATE ${GTK_LIBRARY_DIRS})
endif()

if(MSVC)
    target_compile_options(CSim PRIVATE
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/fsanitize=address>
        $<$<CONFIG:Release>:/O2>
    )
else()
    target_compile_options(CSim PRIVATE
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
# Headless unit / e2e tests (no real window; MockBackend inject)
# ---------------------------------------------------------------------------
enable_testing()

add_executable(CSimTests
	${CSIM_TEST_SOURCES}
	${CSIM_SHARED_SOURCES}
)
csim_configure_target(CSimTests)

# Keep old target name as an alias so existing scripts keep working.
add_executable(CSimRenderTests ALIAS CSimTests)

add_test(NAME CSimTests COMMAND CSimTests)
# Backward-compatible CTest name
add_test(NAME MockBackend COMMAND CSimTests)
set_tests_properties(CSimTests MockBackend PROPERTIES
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Run the current headless suite as part of the default build.  Keep this
# separate from the CTest registration so explicit target builds remain useful
# while a normal build still verifies the executable that was just built.
add_custom_target(CSimRunTests ALL
	COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> -R "^CSimTests$" --output-on-failure
	DEPENDS CSimTests
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	USES_TERMINAL
	COMMENT "Running current CSimTests suite"
)
