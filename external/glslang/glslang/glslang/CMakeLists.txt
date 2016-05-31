cmake_minimum_required (VERSION 3.0.2)

project (glslang CXX)

set(SRC
    MachineIndependent/glslang_tab.cpp
    MachineIndependent/Constant.cpp
    MachineIndependent/InfoSink.cpp
    MachineIndependent/Initialize.cpp
    MachineIndependent/IntermTraverse.cpp
    MachineIndependent/Intermediate.cpp
    MachineIndependent/ParseHelper.cpp
    MachineIndependent/PoolAlloc.cpp
    MachineIndependent/RemoveTree.cpp
    MachineIndependent/Scan.cpp
    MachineIndependent/ShaderLang.cpp
    MachineIndependent/SymbolTable.cpp
    MachineIndependent/Versions.cpp
    MachineIndependent/intermOut.cpp
    MachineIndependent/limits.cpp
    MachineIndependent/linkValidate.cpp
    MachineIndependent/parseConst.cpp
    MachineIndependent/reflection.cpp
    MachineIndependent/preprocessor/Pp.cpp
    MachineIndependent/preprocessor/PpAtom.cpp
    MachineIndependent/preprocessor/PpContext.cpp
    MachineIndependent/preprocessor/PpMemory.cpp
    MachineIndependent/preprocessor/PpScanner.cpp
    MachineIndependent/preprocessor/PpSymbols.cpp
    MachineIndependent/preprocessor/PpTokens.cpp
    MachineIndependent/propagateNoContraction.cpp
    GenericCodeGen/CodeGen.cpp
    GenericCodeGen/Link.cpp
)

if (WIN32)
    add_subdirectory (OSDependent/Windows)
elseif (UNIX)
    add_subdirectory (OSDependent/Unix)
endif (WIN32)

set(HEADERS
    Public/ShaderLang.h
    Include/arrays.h
    Include/BaseTypes.h
    Include/Common.h
    Include/ConstantUnion.h
    Include/InfoSink.h
    Include/InitializeGlobals.h
    Include/intermediate.h
    Include/PoolAlloc.h
    Include/ResourceLimits.h
    Include/revision.h
    Include/ShHandle.h
    Include/Types.h
    MachineIndependent/glslang_tab.cpp.h
    MachineIndependent/gl_types.h
    MachineIndependent/Initialize.h
    MachineIndependent/localintermediate.h
    MachineIndependent/ParseHelper.h
    MachineIndependent/reflection.h
    MachineIndependent/RemoveTree.h
    MachineIndependent/Scan.h
    MachineIndependent/ScanContext.h
    MachineIndependent/SymbolTable.h
    MachineIndependent/Versions.h
    MachineIndependent/parseVersions.h
    MachineIndependent/propagateNoContraction.h
    MachineIndependent/preprocessor/PpContext.h
    MachineIndependent/preprocessor/PpTokens.h
)

add_library(glslang STATIC ${SRC} ${HEADERS})

target_compile_options (
  glslang PUBLIC
  "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fPIC -std=c++11>"
  "$<$<CXX_COMPILER_ID:MSVC>:/MP>"
)

target_compile_definitions (
	glslang PUBLIC
	"$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:GLSLANG_OSINCLUDE_UNIX>"
	"$<$<CXX_COMPILER_ID:MSVC>:GLSLANG_OSINCLUDE_WIN32>"
)

set_property (TARGET glslang PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property (TARGET glslang PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set_property (TARGET glslang PROPERTY PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

source_group("Public" REGULAR_EXPRESSION "Public/*")
source_group("MachineIndependent" REGULAR_EXPRESSION "MachineIndependent/[^/]*")
source_group("Include" REGULAR_EXPRESSION "Include/[^/]*")
source_group("GenericCodeGen" REGULAR_EXPRESSION "GenericCodeGen/*")
source_group("MachineIndependent\\Preprocessor" REGULAR_EXPRESSION "MachineIndependent/preprocessor/*")
