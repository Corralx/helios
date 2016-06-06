#pragma once

#include <chrono>
using namespace std::literals::chrono_literals;
#include <cstdint>
#include <limits>
#include <vector>
#include <experimental/filesystem>
namespace fs { using namespace std::experimental::filesystem::v1; }

/* NOTE(Corralx): The following section is used as a unified place to include
 * some dependencies which triggers a lot of warning on some compilers (especially Clang).
 * This avoid duplicates of the pragma directives to disable them.
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wweak-vtables"
#include "SDL.h"
#include "SDL_syswm.h"
#include "imgui/imgui.h"
#include "imgui_sdl_bridge.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "ShaderLang.h"
#include "GL/gl3w.h"
#pragma clang diagnostic pop

#pragma warning(push, 0)
#include "rapidjson/document.h"
#pragma warning(pop)
/* --------------------------------------------------------------------------- */

std::string get_content_of_file(const fs::path& path);

enum class shader_type : uint32_t
{
	VERTEX = GL_VERTEX_SHADER,
	GEOMETRY = GL_GEOMETRY_SHADER,
	FRAGMENT = GL_FRAGMENT_SHADER,
	COMPUTE = GL_COMPUTE_SHADER
};

static constexpr uint32_t invalid_handle = std::numeric_limits<uint32_t>::max();
static constexpr int32_t invalid_location = std::numeric_limits<int32_t>::max();

// NOTE(Corralx): An active GL context is required on the calling thread for these to work
uint32_t compile_shader(const std::string& source, shader_type type);
uint32_t link_program(std::vector<uint32_t> shaders);

#ifdef _DEBUG
void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* user_param);
#endif
