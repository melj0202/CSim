cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ILLUMO_DIR)
    set(ILLUMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

project(Illumo)

find_package(OpenGL REQUIRED)

# Include Third-Party Sources
add_subdirectory("thirdparty/glfw-3.4" SYSTEM)
add_subdirectory("thirdparty/glm" SYSTEM)
add_subdirectory("thirdparty/freetype-2.13.3" SYSTEM)

# Static GLEW from vendored sources. Prefer this over the upstream
# thirdparty/glew-2.1.0/build/cmake tree, which is not always present in the
# sparse/vendor checkout used by this repository.
if(NOT TARGET glew_s)
    add_library(glew_s STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glew-2.1.0/src/glew.c"
    )
    target_include_directories(glew_s PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glew-2.1.0/include"
    )
    target_compile_definitions(glew_s PUBLIC GLEW_STATIC)
    target_compile_definitions(glew_s PRIVATE GLEW_NO_GLU)
    if(MSVC)
        target_compile_definitions(glew_s PRIVATE VC_EXTRALEAN)
        target_compile_options(glew_s PRIVATE /GS-)
    endif()
    target_link_libraries(glew_s PUBLIC OpenGL::GL)
    set_target_properties(glew_s PROPERTIES
        OUTPUT_NAME "libglew32"
        FOLDER "thirdparty"
    )
endif()

# Sources compiled by both the application and the headless test executable.
# Keep this as a source list instead of a static library: Illumo enables Tracy in
# Debug while IllumoTests intentionally compiles these files without Tracy.
set(ILLUMO_SHARED_SOURCES
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
    Source/Services/InputManager.cpp
    Source/Rendering/GLString.cpp
    Source/Rendering/Camera.cpp
)

set(ILLUMO_APP_SOURCES
    ${ILLUMO_SHARED_SOURCES}
    Source/Rulesets/Rule90RuleSet.cpp
    Source/Rulesets/Rule184RuleSet.cpp
    Source/Foundation/SysInfo.cpp
    Source/Assets/TTFLoader.cpp
    Source/Services/DebugAlloc.cpp
    Source/App/CellMain.cpp
    Source/Engine/Illumo.cpp
    Source/Rendering/RenderWindow.cpp
    Source/Rendering/OpenGL/CreateOpenGLBackend.cpp
    Source/Rendering/OpenGL/GLBackend.cpp
    Source/Rendering/OpenGL/GLBuffer.cpp
    Source/Rendering/OpenGL/GLDevice.cpp
    thirdparty/tracy-0.13.1/public/TracyClient.cpp
)

# Production features that are also exercised headlessly. Keep platform save
# dialogs out of this list; the test target supplies deterministic SaveLoad
# implementations while the application links its native platform source.
set(ILLUMO_HEADLESS_FEATURE_SOURCES
    Source/Game/CellGameModule.cpp
    Source/Services/SysCmdLine.cpp
    Source/Rendering/SplashText.cpp
)

list(APPEND ILLUMO_APP_SOURCES ${ILLUMO_HEADLESS_FEATURE_SOURCES})

set(ILLUMO_TEST_SOURCES
    Source/Tests/TestMain.cpp
    Source/Tests/TestMockBackend.cpp
    Source/Tests/TestRendererE2E.cpp
    Source/Tests/TestRuleSets.cpp
    Source/Tests/TestCellContext.cpp
    Source/Tests/TestCanvasDomain.cpp
    Source/Tests/TestUITokens.cpp
    Source/Tests/TestServices.cpp
    Source/Tests/TestCellGameModule.cpp
    Source/Tests/TestRuntimeUtilities.cpp
)

# Settings that truly are common to production and test targets. Application-
# only profiling, optimization, and sanitizer flags remain on Illumo below.
function(illumo_configure_target target_name)
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
        glm::glm
    )

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /FS /MP)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra)
    endif()
endfunction()

# Select only the platform entry points here; the executable is defined once.
set(ILLUMO_PLATFORM_SOURCES)
set(ILLUMO_PLATFORM_LIBRARIES)

if(WIN32)
    list(APPEND ILLUMO_PLATFORM_SOURCES
        Source/Platform/Windows/WinMain.cpp
        Source/Platform/Windows/WinSaveLoad.cpp
    )
elseif(APPLE)
    list(APPEND ILLUMO_PLATFORM_SOURCES
        Source/Platform/macOS/Main.cpp
        Source/Platform/macOS/MacSaveLoad.mm
    )
    list(APPEND ILLUMO_PLATFORM_LIBRARIES
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK REQUIRED gtkmm-3.0)
    list(APPEND ILLUMO_PLATFORM_SOURCES
        Source/Platform/Linux/_main.cpp
        Source/Platform/Linux/LinuxSaveLoad.cpp
    )
    list(APPEND ILLUMO_PLATFORM_LIBRARIES ${GTK_LIBRARIES})
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

add_executable(Illumo
    ${ILLUMO_APP_SOURCES}
    ${ILLUMO_PLATFORM_SOURCES}
)
target_sources(Illumo PRIVATE
    $<$<CONFIG:Debug>:Source/Engine/DebugModule.cpp>
)
illumo_configure_target(Illumo)

target_link_libraries(Illumo PRIVATE
	glew_s
	glfw
	OpenGL::GL
    freetype-interface
    freetype
    ${ILLUMO_PLATFORM_LIBRARIES}
)

# Keep the notices required by the libraries and font used at runtime beside
# distributable binaries. The source-tree inventory also documents vendored
# packages that are not linked by the current targets.
add_custom_command(TARGET Illumo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Illumo>/licenses"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../THIRD_PARTY_NOTICES.md"
        "$<TARGET_FILE_DIR:Illumo>/THIRD_PARTY_NOTICES.md"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/freetype-2.13.3/LICENSE.TXT"
        "$<TARGET_FILE_DIR:Illumo>/licenses/FreeType-LICENSE.TXT"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/freetype-2.13.3/docs/FTL.TXT"
        "$<TARGET_FILE_DIR:Illumo>/licenses/FreeType-FTL.TXT"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glew-2.1.0/LICENSE.txt"
        "$<TARGET_FILE_DIR:Illumo>/licenses/GLEW-LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw-3.4/LICENSE.md"
        "$<TARGET_FILE_DIR:Illumo>/licenses/GLFW-LICENSE.md"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm/copying.txt"
        "$<TARGET_FILE_DIR:Illumo>/licenses/GLM-LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/json/LICENSE.MIT"
        "$<TARGET_FILE_DIR:Illumo>/licenses/nlohmann-json-LICENSE.MIT"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb/LICENSE"
        "$<TARGET_FILE_DIR:Illumo>/licenses/stb-LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tracy-0.13.1/LICENSE"
        "$<TARGET_FILE_DIR:Illumo>/licenses/Tracy-LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Fonts/Handjet/OFL.txt"
        "$<TARGET_FILE_DIR:Illumo>/licenses/Handjet-OFL.txt"
    COMMENT "Copying third-party notices beside Illumo"
)

if(UNIX AND NOT APPLE)
    target_compile_options(Illumo PRIVATE ${GTK_CFLAGS_OTHER})
    target_include_directories(Illumo SYSTEM PRIVATE ${GTK_INCLUDE_DIRS})
    target_link_directories(Illumo PRIVATE ${GTK_LIBRARY_DIRS})
endif()

if(MSVC)
    target_compile_options(Illumo PRIVATE
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/fsanitize=address>
        $<$<CONFIG:Release>:/O2>
    )
else()
    target_compile_options(Illumo PRIVATE
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-fsanitize=address>
        $<$<CONFIG:Release>:-O3>
    )
endif()

target_compile_definitions(Illumo PRIVATE
    $<$<CONFIG:Debug>:TRACY_ENABLE>
)

# Copy over the shader files to build execution paths
add_custom_command(TARGET Illumo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Shader"
        "$<TARGET_FILE_DIR:Illumo>/Shader"
    COMMENT "Copying Shader directory beside Illumo"
)

# ---------------------------------------------------------------------------
# Headless unit / e2e tests (no real window; MockBackend inject)
# ---------------------------------------------------------------------------
enable_testing()

add_executable(IllumoTests
	${ILLUMO_TEST_SOURCES}
	${ILLUMO_SHARED_SOURCES}
	${ILLUMO_HEADLESS_FEATURE_SOURCES}
)
illumo_configure_target(IllumoTests)
target_compile_definitions(IllumoTests PRIVATE ILLUMO_TESTING)
target_link_libraries(IllumoTests PRIVATE glfw)

add_custom_command(TARGET IllumoTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Shader"
        "$<TARGET_FILE_DIR:IllumoTests>/Shader"
    COMMENT "Copying Shader directory beside IllumoTests"
)

if(MSVC)
    get_filename_component(MSVC_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_file(ASAN_DLL "clang_rt.asan_dynamic-x86_64.dll"
        PATHS "${MSVC_BIN_DIR}"
    )
    if(ASAN_DLL)
        add_custom_command(TARGET Illumo POST_BUILD
            COMMAND $<$<CONFIG:Debug>:${CMAKE_COMMAND}> $<$<CONFIG:Debug>:-E> $<$<CONFIG:Debug>:copy_if_different>
                "$<$<CONFIG:Debug>:${ASAN_DLL}>"
                "$<$<CONFIG:Debug>:$<TARGET_FILE_DIR:Illumo>>"
            COMMENT "Copying ASan DLL beside Illumo Debug executable"
        )
        add_custom_command(TARGET IllumoTests POST_BUILD
            COMMAND $<$<CONFIG:Debug>:${CMAKE_COMMAND}> $<$<CONFIG:Debug>:-E> $<$<CONFIG:Debug>:copy_if_different>
                "$<$<CONFIG:Debug>:${ASAN_DLL}>"
                "$<$<CONFIG:Debug>:$<TARGET_FILE_DIR:IllumoTests>>"
            COMMENT "Copying ASan DLL beside IllumoTests Debug executable"
        )
    endif()
endif()

option(ILLUMO_ENABLE_COVERAGE "Instrument IllumoTests and add the IllumoCoverage target" OFF)
if(ILLUMO_ENABLE_COVERAGE)
	if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		message(FATAL_ERROR "ILLUMO_ENABLE_COVERAGE requires Clang/LLVM")
	endif()

	target_compile_options(IllumoTests PRIVATE
		-fprofile-instr-generate
		-fcoverage-mapping
	)
	target_link_options(IllumoTests PRIVATE -fprofile-instr-generate)

	find_program(ILLUMO_LLVM_PROFDATA_EXECUTABLE NAMES llvm-profdata)
	find_program(ILLUMO_LLVM_COV_EXECUTABLE NAMES llvm-cov)
	if(NOT ILLUMO_LLVM_PROFDATA_EXECUTABLE OR NOT ILLUMO_LLVM_COV_EXECUTABLE)
		message(FATAL_ERROR "IllumoCoverage requires llvm-profdata and llvm-cov on PATH")
	endif()
endif()

# Convenience alias for render-focused test builds; IllumoTests is canonical.
add_executable(IllumoRenderTests ALIAS IllumoTests)

# Generate one CTest entry per logical case after the runner is built. The
# executable remains shared for compile efficiency, but CTest invokes exactly
# one named case per process so failures are isolated and individually visible.
set(ILLUMO_TEST_DISCOVERY_FILE
	"${CMAKE_CURRENT_BINARY_DIR}/IllumoTests-discovered.cmake"
)
file(WRITE "${ILLUMO_TEST_DISCOVERY_FILE}"
	"# IllumoTests discovery is populated after the test runner builds.\n"
)
add_custom_command(TARGET IllumoTests POST_BUILD
	COMMAND $<TARGET_FILE:IllumoTests>
		--write-ctest
		"${ILLUMO_TEST_DISCOVERY_FILE}"
		$<TARGET_FILE:IllumoTests>
		"${CMAKE_CURRENT_BINARY_DIR}"
	VERBATIM
)
set_property(DIRECTORY APPEND PROPERTY
	TEST_INCLUDE_FILES "${ILLUMO_TEST_DISCOVERY_FILE}"
)

# Run the current headless suite as part of the default build.  Keep this
# separate from the CTest registration so explicit target builds remain useful
# while a normal build still verifies the executable that was just built.
add_custom_target(IllumoRunTests ALL
	COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> -L Illumo --output-on-failure
	DEPENDS IllumoTests
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	USES_TERMINAL
	COMMENT "Running individually registered Illumo test cases"
)

if(ILLUMO_ENABLE_COVERAGE)
	add_custom_target(IllumoCoverage
		COMMAND ${CMAKE_COMMAND}
			"-DTEST_BINARY=$<TARGET_FILE:IllumoTests>"
			"-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
			"-DCTEST_COMMAND=${CMAKE_CTEST_COMMAND}"
			"-DCONFIG=$<CONFIG>"
			"-DLLVM_PROFDATA=${ILLUMO_LLVM_PROFDATA_EXECUTABLE}"
			"-DLLVM_COV=${ILLUMO_LLVM_COV_EXECUTABLE}"
			"-DMINIMUM_LINE_COVERAGE=85"
			-P "${CMAKE_CURRENT_LIST_DIR}/cmake/RunCoverage.cmake"
		DEPENDS IllumoTests
		USES_TERMINAL
		COMMENT "Running granular tests and enforcing 85 percent production line coverage"
	)
endif()

# Build the living documentation with normal all-target builds when the local
# toolchain provides TeX. Focused application/test targets remain usable on
# machines without documentation tooling.
option(ILLUMO_BUILD_DOCUMENTATION "Build the Illumo PDF with the default build" ON)
if(ILLUMO_BUILD_DOCUMENTATION)
    find_program(ILLUMO_POWERSHELL_EXECUTABLE NAMES powershell pwsh)
    find_program(ILLUMO_LATEXMK_EXECUTABLE NAMES latexmk)

    if(WIN32 AND ILLUMO_POWERSHELL_EXECUTABLE AND ILLUMO_LATEXMK_EXECUTABLE)
        add_custom_target(IllumoDocs ALL
            COMMAND "${ILLUMO_POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_LIST_DIR}/../docs/build.ps1"
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../docs"
            USES_TERMINAL
            COMMENT "Building Illumo documentation"
        )
    else()
        message(STATUS "Illumo documentation target skipped (requires Windows PowerShell and latexmk; set ILLUMO_BUILD_DOCUMENTATION=OFF to silence this message)")
    endif()
endif()
