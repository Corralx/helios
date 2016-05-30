set(SRC InitializeDll.cpp InitializeDll.h)

add_library(OGLCompiler STATIC ${SRC})

set_property (TARGET OGLCompiler PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property (TARGET OGLCompiler PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set_property (TARGET OGLCompiler PROPERTY PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

source_group("Source" FILES ${SOURCES})
