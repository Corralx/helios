set(SRC ossource.cpp ../osinclude.h)

add_library(OSDependent STATIC ${SRC})

source_group("Source" FILES ${SRC})

set_property (TARGET OSDependent PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property (TARGET OSDependent PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set_property (TARGET OSDependent PROPERTY PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
