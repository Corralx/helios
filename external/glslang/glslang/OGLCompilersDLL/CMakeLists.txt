cmake_minimum_required (VERSION 3.0.2)

project (OGLCompiler CXX)

set(SRC InitializeDll.cpp InitializeDll.h)

add_library(OGLCompiler STATIC ${SRC})

target_compile_options (
  OGLCompiler PRIVATE
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-std=c++11>
)

set_property (TARGET OGLCompiler PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property (TARGET OGLCompiler PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set_property (TARGET OGLCompiler PROPERTY PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

source_group("Source" FILES ${SOURCES})
