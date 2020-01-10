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
	${CMAKE_CURRENT_SOURCE_DIR}/src/vcl/graphics/recorder/recorder.h
)
set(VCL_RECORDER_PUB_SRC
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
	file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.14/conan.cmake" "${CMAKE_BINARY_DIR}/conan.cmake")
endif()
include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_run(
	REQUIRES
		ffmpeg/4.0.2@bincrafters/stable
		glew/2.1.0@bincrafters/stable
		glfw/3.2.1@bincrafters/stable
		imgui/1.63@bincrafters/stable
	BASIC_SETUP
	CMAKE_TARGETS
	#BUILD
	#	missing
)

function(remap_target TGT)
	# Remap build types for external targets to avoid non-existing builds
	set_target_properties(${TGT}
		PROPERTIES
			MAP_IMPORTED_CONFIG_MINSIZEREL Release
			MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
	)
	message(STATUS "Remapped configurations for ${TGT}")

	get_target_property(CONAN_IMPORT_TARGETS ${TGT} INTERFACE_LINK_LIBRARIES)
	list(FILTER CONAN_IMPORT_TARGETS INCLUDE REGEX CONAN_PKG::.+)

	foreach(IMPORTED_TARGET ${CONAN_IMPORT_TARGETS})
		remap_target(${IMPORTED_TARGET})
	endforeach()
endfunction()

set(EXTERNAL_TARGETS CONAN_PKG::ffmpeg CONAN_PKG::glew CONAN_PKG::glfw CONAN_PKG::imgui)
foreach(tgt ${EXTERNAL_TARGETS})
	remap_target(${tgt})
endforeach()

target_link_libraries(vcl.graphics.recorder
	PRIVATE
		CONAN_PKG::ffmpeg
	PUBLIC
		absl::strings
		GSL
)

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

option(VCL_BUILD_EXAMPLES "Build the examples" OFF)
if (VCL_BUILD_EXAMPLES)

	set(EXAMPLES_SRC
		examples/screenrecorder/main.cpp
	)
	source_group("" FILES ${EXAMPLES_SRC})
	set(EXAMPLES_IMGUI_SRC
		examples/screenrecorder/imgui/imgui_impl_glfw.cpp
		examples/screenrecorder/imgui/imgui_impl_glfw.h
		examples/screenrecorder/imgui/imgui_impl_opengl3.cpp
		examples/screenrecorder/imgui/imgui_impl_opengl3.h
	)
	source_group("imgui" FILES ${EXAMPLES_IMGUI_SRC})

	add_executable(screenrecorder
		${EXAMPLES_SRC}
		${EXAMPLES_IMGUI_SRC}
	)

	target_link_libraries(screenrecorder
		PRIVATE
			vcl.graphics.recorder
			CONAN_PKG::glew
			CONAN_PKG::glfw
			CONAN_PKG::imgui
	)
	
endif (VCL_BUILD_EXAMPLES)
