#include "common.hpp"

#include <fstream>
#include <iostream>
#include <string>

std::string get_content_of_file(const fs::path& path)
{
	std::ifstream stream(path);
	if (!stream.is_open() || !stream.good())
		return "";

	std::string content(std::istreambuf_iterator<char>(stream),
						(std::istreambuf_iterator<char>()));

	return content;
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
		glDeleteShader(shader);
		return invalid_handle;
	}

	return shader;
}

uint32_t link_program(std::vector<uint32_t> shaders)
{
	uint32_t program = glCreateProgram();
	
	for (auto shader : shaders)
		glAttachShader(program, shader);

	glLinkProgram(program);

	int32_t success = false;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glDeleteProgram(program);
		return invalid_handle;
	}

	return program;
}

#ifdef _DEBUG
void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* user_param)
{
	user_param;
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
