# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.29

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/jaskulr/.local/share/JetBrains/Toolbox/apps/clion/bin/cmake/linux/x64/bin/cmake

# The command to remove a file.
RM = /home/jaskulr/.local/share/JetBrains/Toolbox/apps/clion/bin/cmake/linux/x64/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/jaskulr/Source/Repos/CSim/CSim

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug

# Include any dependencies generated for this target.
include thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/compiler_depend.make

# Include the progress variables for this target.
include thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/progress.make

# Include the compile flags for this target's objects.
include thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/flags.make

thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.o: thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/flags.make
thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.o: /home/jaskulr/Source/Repos/CSim/CSim/thirdparty/glew-2.1.0/src/visualinfo.c
thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.o: thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.o"
	cd /home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug/thirdparty/glew-2.1.0/build/cmake && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.o -MF CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.o.d -o CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.o -c /home/jaskulr/Source/Repos/CSim/CSim/thirdparty/glew-2.1.0/src/visualinfo.c

thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.i"
	cd /home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug/thirdparty/glew-2.1.0/build/cmake && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jaskulr/Source/Repos/CSim/CSim/thirdparty/glew-2.1.0/src/visualinfo.c > CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.i

thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.s"
	cd /home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug/thirdparty/glew-2.1.0/build/cmake && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jaskulr/Source/Repos/CSim/CSim/thirdparty/glew-2.1.0/src/visualinfo.c -o CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.s

# Object files for target visualinfo
visualinfo_OBJECTS = \
"CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.o"

# External object files for target visualinfo
visualinfo_EXTERNAL_OBJECTS =

bin/visualinfo: thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/__/__/src/visualinfo.c.o
bin/visualinfo: thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/build.make
bin/visualinfo: lib/libGLEWd.so.2.1.0
bin/visualinfo: /usr/lib/x86_64-linux-gnu/libSM.so
bin/visualinfo: /usr/lib/x86_64-linux-gnu/libICE.so
bin/visualinfo: /usr/lib/x86_64-linux-gnu/libX11.so
bin/visualinfo: /usr/lib/x86_64-linux-gnu/libXext.so
bin/visualinfo: /usr/lib/x86_64-linux-gnu/libOpenGL.so
bin/visualinfo: /usr/lib/x86_64-linux-gnu/libGLX.so
bin/visualinfo: /usr/lib/x86_64-linux-gnu/libGLU.so
bin/visualinfo: thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable ../../../../bin/visualinfo"
	cd /home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug/thirdparty/glew-2.1.0/build/cmake && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/visualinfo.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/build: bin/visualinfo
.PHONY : thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/build

thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/clean:
	cd /home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug/thirdparty/glew-2.1.0/build/cmake && $(CMAKE_COMMAND) -P CMakeFiles/visualinfo.dir/cmake_clean.cmake
.PHONY : thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/clean

thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/depend:
	cd /home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jaskulr/Source/Repos/CSim/CSim /home/jaskulr/Source/Repos/CSim/CSim/thirdparty/glew-2.1.0/build/cmake /home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug /home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug/thirdparty/glew-2.1.0/build/cmake /home/jaskulr/Source/Repos/CSim/CSim/cmake-build-debug/thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : thirdparty/glew-2.1.0/build/cmake/CMakeFiles/visualinfo.dir/depend
