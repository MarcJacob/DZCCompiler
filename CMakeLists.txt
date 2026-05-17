cmake_minimum_required (VERSION 3.8)

project ("DZCCompiler")

# Single translation unit.
add_executable (DZCCompiler "src/main.c")

# Set standard to C99.
set_property(TARGET DZCCompiler PROPERTY C_STANDARD 99)

# Add ./include/ folder as include target.
target_include_directories(DZCCompiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

# Use all warnings, and selectively ignore some.
target_compile_options(DZCCompiler PRIVATE /Wall /wd4061 /wd4820 /wd5045)

# If DEBUG_SANITIZE_BUILD is set and true, use address sanitizer.
if (DEBUG_SANITIZE_BUILD)
	target_compile_options(DZCCompiler PRIVATE /fsanitize=address)
endif()
