cmake_minimum_required (VERSION 3.0.2)

project (helios)

set (SDL2_INCLUDE_PATH "" CACHE PATH "SDL2 include path")
set (SDL2_LIB_PATH "" CACHE PATH "SDL2 library path")


set (GL3W_PATH ${CMAKE_SOURCE_DIR}/external/gl3w)
set (GLM_PATH ${CMAKE_SOURCE_DIR}/external/glm)
set (TCLAP_PATH ${CMAKE_SOURCE_DIR}/external/tclap)
set (CPPFORMAT_PATH ${CMAKE_SOURCE_DIR}/external/cppformat)
set (IMGUI_PATH ${CMAKE_SOURCE_DIR}/external/imgui)
set (RAPIDJSON_PATH ${CMAKE_SOURCE_DIR}/external/rapidjson)

set (
	GL3W_INCLUDE_PATH
	${GL3W_PATH}/include
)

set (
	GLM_INCLUDE_PATH
	${GLM_PATH}
)

set (
	TCLAP_INCLUDE_PATH
	${TCLAP_PATH}
)

set (
	CPPFORMAT_INCLUDE_PATH
	${CPPFORMAT_PATH}
)

set (
	IMGUI_INCLUDE_PATH
	${IMGUI_PATH}/include
)

set (
	RAPIDJSON_INCLUDE_PATH
	${RAPIDJSON_PATH}
)

add_subdirectory (${GL3W_PATH})
add_subdirectory (${CPPFORMAT_PATH})
add_subdirectory (${IMGUI_PATH})
add_subdirectory (helios)
