#pragma once

#include <cstdint>
#include <limits>
#include <vector>
#include <experimental/filesystem>

#include "GL/gl3w.h"

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

// NOTE(Corralx): An active GL context is required on the calling thread for these to work
uint32_t compile_shader(const std::string& source, shader_type type);
uint32_t link_program(std::vector<uint32_t> shaders);

#ifdef _DEBUG
void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* user_param);
#endif
