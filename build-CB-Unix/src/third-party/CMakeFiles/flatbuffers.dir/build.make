# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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

# Produce verbose output by default.
VERBOSE = 1

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/point/OpenSource/Cataclysm-DDA

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix

# Include any dependencies generated for this target.
include src/third-party/CMakeFiles/flatbuffers.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/third-party/CMakeFiles/flatbuffers.dir/compiler_depend.make

# Include the progress variables for this target.
include src/third-party/CMakeFiles/flatbuffers.dir/progress.make

# Include the compile flags for this target's objects.
include src/third-party/CMakeFiles/flatbuffers.dir/flags.make

src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.o: src/third-party/CMakeFiles/flatbuffers.dir/flags.make
src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.o: ../src/third-party/flatbuffers/idl_parser.cpp
src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.o: src/third-party/CMakeFiles/flatbuffers.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.o"
	cd /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/src/third-party && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.o -MF CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.o.d -o CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.o -c /home/point/OpenSource/Cataclysm-DDA/src/third-party/flatbuffers/idl_parser.cpp

src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.i"
	cd /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/src/third-party && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/point/OpenSource/Cataclysm-DDA/src/third-party/flatbuffers/idl_parser.cpp > CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.i

src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.s"
	cd /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/src/third-party && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/point/OpenSource/Cataclysm-DDA/src/third-party/flatbuffers/idl_parser.cpp -o CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.s

src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.o: src/third-party/CMakeFiles/flatbuffers.dir/flags.make
src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.o: ../src/third-party/flatbuffers/util.cpp
src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.o: src/third-party/CMakeFiles/flatbuffers.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.o"
	cd /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/src/third-party && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.o -MF CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.o.d -o CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.o -c /home/point/OpenSource/Cataclysm-DDA/src/third-party/flatbuffers/util.cpp

src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.i"
	cd /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/src/third-party && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/point/OpenSource/Cataclysm-DDA/src/third-party/flatbuffers/util.cpp > CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.i

src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.s"
	cd /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/src/third-party && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/point/OpenSource/Cataclysm-DDA/src/third-party/flatbuffers/util.cpp -o CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.s

# Object files for target flatbuffers
flatbuffers_OBJECTS = \
"CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.o" \
"CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.o"

# External object files for target flatbuffers
flatbuffers_EXTERNAL_OBJECTS =

src/third-party/libflatbuffers.a: src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/idl_parser.cpp.o
src/third-party/libflatbuffers.a: src/third-party/CMakeFiles/flatbuffers.dir/flatbuffers/util.cpp.o
src/third-party/libflatbuffers.a: src/third-party/CMakeFiles/flatbuffers.dir/build.make
src/third-party/libflatbuffers.a: src/third-party/CMakeFiles/flatbuffers.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library libflatbuffers.a"
	cd /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/src/third-party && $(CMAKE_COMMAND) -P CMakeFiles/flatbuffers.dir/cmake_clean_target.cmake
	cd /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/src/third-party && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/flatbuffers.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/third-party/CMakeFiles/flatbuffers.dir/build: src/third-party/libflatbuffers.a
.PHONY : src/third-party/CMakeFiles/flatbuffers.dir/build

src/third-party/CMakeFiles/flatbuffers.dir/clean:
	cd /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/src/third-party && $(CMAKE_COMMAND) -P CMakeFiles/flatbuffers.dir/cmake_clean.cmake
.PHONY : src/third-party/CMakeFiles/flatbuffers.dir/clean

src/third-party/CMakeFiles/flatbuffers.dir/depend:
	cd /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/point/OpenSource/Cataclysm-DDA /home/point/OpenSource/Cataclysm-DDA/src/third-party /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/src/third-party /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/src/third-party/CMakeFiles/flatbuffers.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/third-party/CMakeFiles/flatbuffers.dir/depend

