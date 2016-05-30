#include "common.hpp"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <regex>
#include "GL/gl3w.h"

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
	{
		int maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
		std::cout << &errorLog[0] << std::endl;

		return invalid_handle;
	}
	
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

static const std::unordered_map<std::string, uniform_type> uniform_type_dict =
{
	{ "float", uniform_type::FLOAT },
	{ "vec2", uniform_type::VEC2 },
	{ "vec3", uniform_type::VEC3 },
	{ "vec4", uniform_type::VEC4 },
	{ "bool", uniform_type::BOOL },
	{ "bvec2", uniform_type::BVEC2 },
	{ "bvec3", uniform_type::BVEC3 },
	{ "bvec4", uniform_type::BVEC4 },
	{ "int", uniform_type::INT },
	{ "ivec2", uniform_type::IVEC2 },
	{ "ivec3", uniform_type::IVEC3 },
	{ "ivec4", uniform_type::IVEC4 },
	{ "uint", uniform_type::UINT },
	{ "uvec2", uniform_type::UVEC2 },
	{ "uvec3", uniform_type::UVEC3 },
	{ "uvec4", uniform_type::UVEC4 },
	{ "double", uniform_type::DOUBLE },
	{ "dvec2", uniform_type::DVEC2 },
	{ "dvec3", uniform_type::DVEC3 },
	{ "dvec4", uniform_type::DVEC4 },
};

// TODO(Corralx): We parse commented uniforms too, but we exclude them later on because they don't have a location
// TODO(Corralx): We do not support vectors, matrices or booleans yet
std::vector<uniform_t> extract_uniform(const std::string& source, uint32_t program)
{
	// NOTE(Corralx): We only parse GLSL code that has already been compiled, so we assume it is correct
	static std::regex uniform_regex(".*uniform\\s*"
									"(float|int|uint|double)\\s*"
									"([a-z0-9_]*)\\s*;\\s*"
									"//\\s*min\\s*:\\s*(-?[0-9]*\\.?[0-9]*)f?\\s*;"
									"\\s*max\\s*:\\s*(-?[0-9]*\\.?[0-9]*)f?\\s*;"
									"\\s*default\\s*:\\s*(-?[0-9]*\\.?[0-9]*)f?\\s*;",
									std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
	std::smatch uniform_match;
	std::vector<uniform_t> uniforms;

	std::string::const_iterator cbegin(source.cbegin());
	while (std::regex_search(cbegin, source.cend(), uniform_match, uniform_regex))
	{
		if (!uniform_match.empty())
		{
			std::string name = uniform_match[2];
			uint32_t location = glGetUniformLocation(program, name.c_str());
			uniform_type type = uniform_type_dict.at(uniform_match[1]);

			float min_value = stof(uniform_match[3]);
			float max_value = stof(uniform_match[4]);
			float value = stof(uniform_match[5]);

			if (location != invalid_handle)
				uniforms.emplace_back(name, location, type, min_value, max_value, value);
			else
				std::cout << "Ignoring uniform \"" << name <<
							 "\" because his location could not be retrived (optimized away?)" << std::endl;
		}

		cbegin += uniform_match.position() + uniform_match.length();
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
