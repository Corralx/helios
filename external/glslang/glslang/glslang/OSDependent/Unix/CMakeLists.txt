cmake_minimum_required (VERSION 3.0.2)

project (OSDependent CXX)

add_library(OSDependent STATIC ossource.cpp ../osinclude.h)

set_property (TARGET OSDependent PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property (TARGET OSDependent PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set_property (TARGET OSDependent PROPERTY PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
