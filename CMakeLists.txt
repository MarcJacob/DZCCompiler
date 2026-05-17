cmake_minimum_required (VERSION 3.8)

project ("DZCCompiler")

# Single translation unit.
add_executable (DZCCompiler "src/main.c")

# 
set_property(TARGET DZCCompiler PROPERTY C_STANDARD 90)

target_include_directories(DZCCompiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

target_compile_options(DZCCompiler PRIVATE /Wall /wd4061 /wd4820 /wd5045)

if (DEBUG_SANITIZE_BUILD)
	target_compile_options(DZCCompiler PRIVATE /fsanitize=address)
endif()
