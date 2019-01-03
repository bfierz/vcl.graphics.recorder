# The VCL screen capture library is released under the MIT license.
# 
# Copyright (c) 2018 Basil Fierz
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
cmake_minimum_required(VERSION 3.0)

# General project configurations
project(vcl.graphics.recorder CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add dependencies
add_subdirectory(externals/abseil EXCLUDE_FROM_ALL)
add_subdirectory(externals/gsl EXCLUDE_FROM_ALL)

add_library(vcl.graphics.recorder SHARED "")

# Define the sources
set(VCL_RECORDER_PRIV_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/src/vcl/graphics/recorder/recorder.cpp
)
set(VCL_RECORDER_PUB_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/src/vcl/graphics/recorder/recorder.h
)
target_sources(vcl.graphics.recorder
	PRIVATE
		${VCL_RECORDER_PRIV_SRC}
	PUBLIC
		${VCL_RECORDER_PUB_SRC}
)
source_group("" FILES ${VCL_RECORDER_PRIV_SRC} ${VCL_RECORDER_PUB_SRC})

target_compile_definitions(vcl.graphics.recorder 
	PRIVATE
		VCL_GRAPHICS_RECORDER_EXPORTS
)

# The include files are stored along-side the source code
target_include_directories(vcl.graphics.recorder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
	message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
	file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake" "${CMAKE_BINARY_DIR}/conan.cmake")
endif()
include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_run(REQUIRES ffmpeg/4.0.2@bincrafters/stable
                BASIC_SETUP CMAKE_TARGETS
                BUILD missing)

target_link_libraries(vcl.graphics.recorder
	PRIVATE
		CONAN_PKG::ffmpeg
	PUBLIC
		absl::strings
		GSL
)

# File System Unit Tests
option(VCL_BUILD_TESTS "Build the unit tests" OFF)
if (VCL_BUILD_TESTS)
	set(BUILD_GTEST ON CACHE BOOL "" FORCE)
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	add_subdirectory(externals/googletest EXCLUDE_FROM_ALL)

	# Define the test files
	set(VCL_TEST_SRC
		tests/empty.cpp
		tests/sequence.cpp
		tests/white.cpp
	)
	source_group("" FILES ${VCL_TEST_SRC})

	add_executable(vcl.graphics.recorder.test
		${VCL_TEST_SRC}
	)

	target_link_libraries(vcl.graphics.recorder.test
		vcl.graphics.recorder
		gtest
		gtest_main
	)
	
endif (VCL_BUILD_TESTS)
