#include "common.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <algorithm>

#include "GL/gl3w.h"
#include "ShaderLang.h"

std::string get_content_of_file(const fs::path& path)
{
	std::ifstream stream(path);
	if (!stream.is_open() || !stream.good())
		return "";

	std::string content(std::istreambuf_iterator<char>(stream),
						(std::istreambuf_iterator<char>()));

	return std::move(content);
}

uint32_t compile_shader(const std::string& source, shader_type type)
{
	uint32_t shader = glCreateShader(static_cast<uint32_t>(type));

	const char* chars = source.c_str();
	glShaderSource(shader, 1, &chars, 0);
	glCompileShader(shader);

	int32_t success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
		return invalid_handle;
	
	return shader;
}

uint32_t link_program(std::initializer_list<uint32_t> shaders)
{
	uint32_t program = glCreateProgram();
	
	for (auto shader : shaders)
		glAttachShader(program, shader);

	glLinkProgram(program);

	int32_t success = false;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
		return invalid_handle;

	return program;
}

// TODO(Corralx): Find a better way to declare these, instead of hardcoding them
static std::vector<std::string> default_uniforms =
{
	"screen_width",
	"screen_height",
	"light_direction",
	"light_color",
	"camera_position",
	"camera_view",
	"camera_up",
	"camera_right",
	"focal_length",
	"time",
	"output_image"
};

// This maps OpenGL type identiers to our enum-based uniform types
static const std::unordered_map<int32_t, uniform_type> uniform_type_dict =
{
	{ GL_FLOAT,				uniform_type::FLOAT	 },
	{ GL_FLOAT_VEC2,		uniform_type::VEC2	 },
	{ GL_FLOAT_VEC3,		uniform_type::VEC3	 },
	{ GL_FLOAT_VEC4,		uniform_type::VEC4	 },
	{ GL_BOOL,				uniform_type::BOOL	 },
	{ GL_BOOL_VEC2,			uniform_type::BVEC2  },
	{ GL_BOOL_VEC3,			uniform_type::BVEC3  },
	{ GL_BOOL_VEC4,			uniform_type::BVEC4  },
	{ GL_INT,				uniform_type::INT	 },
	{ GL_INT_VEC2,			uniform_type::IVEC2  },
	{ GL_INT_VEC3,			uniform_type::IVEC3  },
	{ GL_INT_VEC4,			uniform_type::IVEC4  },
	{ GL_UNSIGNED_INT,		uniform_type::UINT	 },
	{ GL_UNSIGNED_INT_VEC2, uniform_type::UVEC2  },
	{ GL_UNSIGNED_INT_VEC3, uniform_type::UVEC3  },
	{ GL_UNSIGNED_INT_VEC4, uniform_type::UVEC4  },
	{ GL_DOUBLE,			uniform_type::DOUBLE },
	{ GL_DOUBLE_VEC2,		uniform_type::DVEC2	 },
	{ GL_DOUBLE_VEC3,		uniform_type::DVEC3  },
	{ GL_DOUBLE_VEC4,		uniform_type::DVEC4  }
};

using namespace glslang;

// We use C++11 magic statics to automagically manage glslang initialization/destruction
// http://en.cppreference.com/w/cpp/language/storage_duration#Static_local_variables
struct glslang_raii
{
	glslang_raii() { InitializeProcess(); }
	~glslang_raii() { FinalizeProcess(); }
};

std::vector<uniform_t> extract_uniform(const std::string& source)
{
	static glslang_raii glslang_raii{};

	auto shader = std::make_unique<TShader>(EShLangCompute);
	auto source_ptr = source.c_str();
	shader->setStrings(&source_ptr, 1);

	if (!shader->parse(&DefaultTBuiltInResource, 430, false, EShMsgDefault))
	{
		std::cout << "Error compiling scene: " << std::endl;
		std::cout << shader->getInfoLog() << std::endl;
		std::cout << shader->getInfoDebugLog() << std::endl;

		return{};
	}

	auto program = std::make_unique<TProgram>();
	program->addShader(shader.get());

	if (!program->link(EShMsgDefault))
	{
		std::cout << "Error linking program: " << std::endl;
		std::cout << program->getInfoLog() << std::endl;
		std::cout << program->getInfoDebugLog() << std::endl;

		return{};
	}

	// Generate the reflection information for the AST
	program->buildReflection();

	std::vector<uniform_t> uniforms;
	int32_t num_uniforms = program->getNumLiveUniformVariables();
	for (int32_t i = 0; i < num_uniforms; ++i)
	{
		std::string name = program->getUniformName(i);

		// Skip default uniforms
		if (std::find(default_uniforms.begin(), default_uniforms.end(), name) != default_uniforms.end())
			continue;

		int32_t gl_type = program->getUniformType(i);
		uniform_type type = uniform_type_dict.at(gl_type);

		uniforms.emplace_back(name, type);
	}

	return std::move(uniforms);
}

#ifdef _DEBUG
void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* userParam)
{
	userParam;
	length;
	source;

	using namespace std;

	cout << "---------------------opengl-callback-start------------" << endl;
	cout << "message: " << message << endl;
	cout << "type: ";
	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:
			cout << "ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			cout << "DEPRECATED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			cout << "UNDEFINED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			cout << "PORTABILITY";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			cout << "PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_OTHER:
			cout << "OTHER";
			break;
	}
	cout << endl;

	cout << "id: " << id << endl;
	cout << "severity: ";
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_LOW:
			cout << "LOW";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			cout << "MEDIUM";
			break;
		case GL_DEBUG_SEVERITY_HIGH:
			cout << "HIGH";
			break;
	}
	cout << endl;
	cout << "---------------------opengl-callback-end--------------" << endl;
}
#endif
