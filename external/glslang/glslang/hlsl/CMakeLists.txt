cmake_minimum_required (VERSION 3.0.2)

project (HLSL CXX)

set(SRC
    hlslParseHelper.cpp
    hlslScanContext.cpp
    hlslOpMap.cpp
    hlslTokenStream.cpp
    hlslGrammar.cpp
    hlslParseables.cpp)

set(HEADERS
    hlslParseHelper.h
    hlslTokens.h
    hlslScanContext.h
    hlslOpMap.h
    hlslTokenStream.h
    hlslGrammar.h
    hlslParseables.h)

add_library(HLSL STATIC ${SRC} ${HEADERS})

target_compile_options (
  HLSL PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-std=c++11 -fvisibility=default>
  $<$<CXX_COMPILER_ID:Clang>:-std=c++11 -fvisibility=default>
  $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

set_property (TARGET HLSL PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property (TARGET HLSL PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set_property (TARGET HLSL PROPERTY PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

source_group("Source" FILES ${SRC} ${HEADERS})
