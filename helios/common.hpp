#pragma once

#include <cstdint>
#include <limits>
#include <initializer_list>
#include <vector>
#include <experimental/filesystem>

#include "GL/gl3w.h"
#include "glm/glm.hpp"

namespace fs { using namespace std::experimental::filesystem::v1; }
using namespace std::chrono_literals;

std::string get_content_of_file(const fs::path& path);

enum class shader_type : uint32_t
{
	VERTEX = GL_VERTEX_SHADER,
	GEOMETRY = GL_GEOMETRY_SHADER,
	FRAGMENT = GL_FRAGMENT_SHADER,
	COMPUTE = GL_COMPUTE_SHADER
};

static constexpr uint32_t invalid_handle = std::numeric_limits<uint32_t>::max();

enum class uniform_type : uint8_t
{
	FLOAT = 0,
	VEC2,
	VEC3,
	VEC4,

	BOOL,
	BVEC2,
	BVEC3,
	BVEC4,

	INT,
	IVEC2,
	IVEC3,
	IVEC4,

	UINT,
	UVEC2,
	UVEC3,
	UVEC4,

	DOUBLE,
	DVEC2,
	DVEC3,
	DVEC4
};

// TODO(Corralx): Find a way to specify a default value?
// Uniform initialization syntax seems to be bugged in a lot of implementations!
struct uniform_t
{
	uniform_t(const std::string& n, uniform_type t) :
		name(n), location(invalid_handle), type(t), int4(0) {}

	std::string name;
	uint32_t location;
	uniform_type type;

	// NOTE(Corralx): ImGui does not support doubles or unsigned ints
	union
	{
		glm::vec4 float4;
		glm::ivec4 int4;
		glm::bvec4 bool4;
	};
};

// NOTE(Corralx): An active GL context is required on the calling thread for these to work
uint32_t compile_shader(const std::string& source, shader_type type);
uint32_t link_program(std::initializer_list<uint32_t> shaders);

// NOTE(Corralx): After the call, the locations of the uniforms in the real compiled program are still to be retrieved
std::vector<uniform_t> extract_uniform(const std::string& source);

#ifdef _DEBUG
void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* userParam);
#endif
