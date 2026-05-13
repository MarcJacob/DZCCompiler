# CMakeList.txt : CMake project for DZCCompiler, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("DZCCompiler")

# Add source to this project's executable.
add_executable (DZCCompiler "src/main.c" )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DZCCompiler PROPERTY C_STANDARD_LATEST)
endif()

target_include_directories(DZCCompiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

# TODO: Add tests and install targets if needed.
