cmake_minimum_required (VERSION 3.0.2)

project (helios CXX)

set (RESOURCES_DIR "${CMAKE_SOURCE_DIR}/resources")
set (CONFIG_FILE "${RESOURCES_DIR}/config.json")

set (
	SRC
	main.cpp
	common.cpp
	file_watcher.cpp
	imgui_sdl_bridge.cpp
)

set (
	HEADER
	common.hpp
	file_watcher.hpp
	imgui_sdl_bridge.hpp
)

set (
	COMMON
	${CMAKE_SOURCE_DIR}/TODO.md
	${CMAKE_SOURCE_DIR}/LICENSE.md
	${CMAKE_SOURCE_DIR}/README.md
)

set (
	SHADER
	${RESOURCES_DIR}/base_vertex.vert
	${RESOURCES_DIR}/copy.frag
	${RESOURCES_DIR}/raymarch_base.comp
	${RESOURCES_DIR}/raymarch_library.comp
	${RESOURCES_DIR}/raymarch_main.comp
	${RESOURCES_DIR}/raymarch_scene.comp
)

set (
	ALL_FILES
	${SRC}
	${HEADER}
)

foreach (FILE ${ALL_FILES})
  get_filename_component (PARENT_DIR "${FILE}" PATH)

  string (REPLACE "/" "\\" GROUP "${PARENT_DIR}")

  if ("${FILE}" MATCHES ".*\\.cpp")
	set (GROUP "Source Files\\${GROUP}")
  elseif ("${FILE}" MATCHES ".*\\.hpp")
	set (GROUP "Header Files\\${GROUP}")
  endif ()

  source_group ("${GROUP}" FILES "${FILE}")
endforeach ()

source_group ("Resources" FILES ${SHADER} ${CONFIG_FILE})

include_directories (${GL3W_INCLUDE_PATH})
include_directories (${GLM_INCLUDE_PATH})
include_directories (${CPPFORMAT_INCLUDE_PATH})
include_directories (${IMGUI_INCLUDE_PATH})
include_directories (${SDL2_INCLUDE_PATH})
include_directories (${RAPIDJSON_INCLUDE_PATH})

add_executable (
	helios
	${SRC}
	${HEADER}
	${COMMON}
	${SHADER}
	${CONFIG_FILE}
)

set (
	OPENGL_LIB
	gl3w
	$<$<PLATFORM_ID:Windows>:opengl32>
)

add_definitions (-DNOMINMAX)

target_link_libraries (
	helios
	cppformat
	imgui
	${OPENGL_LIB}
	${SDL2_LIB_PATH}/SDL2.lib
	${SDL2_LIB_PATH}/SDL2main.lib
)

set (MSVC_OPTIONS /MP)
set (GNU_OPTIONS -std=c++11)
set (CLANG_OPTIONS -std=c++11)

target_compile_options (
  helios PUBLIC
  "$<$<CXX_COMPILER_ID:MSVC>:${MSVC_OPTIONS}>"
  "$<$<CXX_COMPILER_ID:GNU>:${GNU_OPTIONS}>"
  "$<$<CXX_COMPILER_ID:Clang>:${CLANG_OPTIONS}>"
)

set (CLANG_WARNINGS -Weverything -pedantic -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unknown-pragmas)
set (MSVC_WARNINGS /wd4068 /wd4201 /W4 /WX)
set (GNU_WARNINGS -Wall -Wextra -pedantic -Werror -Wno-pragmas -Wno-unknown-pragmas)

target_compile_options (
  helios PUBLIC 
  "$<$<CXX_COMPILER_ID:MSVC>:${MSVC_WARNINGS}>"
  "$<$<CXX_COMPILER_ID:GNU>:${GNU_WARNINGS}>"
  "$<$<CXX_COMPILER_ID:Clang>:${CLANG_WARNINGS}>"
)

set_property (TARGET helios PROPERTY LINK_FLAGS "/INCREMENTAL:NO")

set_property (TARGET helios PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property (TARGET helios PROPERTY PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

add_custom_command(TARGET helios POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${RESOURCES_DIR} $<TARGET_FILE_DIR:helios>/resources)
