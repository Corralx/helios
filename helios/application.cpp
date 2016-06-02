#include "application.hpp"
#include "common.hpp"

#include <iostream>
#include <cassert>
#include <chrono>
using namespace std::literals::chrono_literals;
using hr_clock = std::chrono::high_resolution_clock;
#include <ctime>
#include <iomanip>

#ifdef _WIN32
#include "Windows.h"
#include "Shellapi.h"
#else
#include <unistd.h>
#endif

#include "imgui/imgui.h"
#include "imgui_sdl_bridge.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

static constexpr uint32_t OPENGL_MAJOR_VERSION = 4;
static constexpr uint32_t OPENGL_MINOR_VERSION = 3;
static constexpr char* WINDOW_NAME = "Helios";

application::application() : _config(), _window(nullptr), _render_context(nullptr), _compiler_context(nullptr),
	_fullscreen_quad(invalid_handle), _offscreen_buffer(invalid_handle), _raymarch_program(invalid_handle),
	_copy_program(invalid_handle), _uniforms(), _should_run(false), _initialized(false), _raymarch_watcher(),
	_temp_program(invalid_handle), _swap_program(false), _raymarch(), _camera(), _light(), _scene(), _postprocess()
{
	// NOTE(Corralx): Nothing to do, everything is postponed to init()
}

bool application::init()
{
	if (_initialized)
		return true;

	_config = load_config();

	bool ret = open_window();
	assert(ret);

	ret &= initialize_opengl();
	assert(ret);

	ret &= create_opengl_resources();
	assert(ret);

	ret &= imgui_init(_window);
	assert(ret);

	if (!ret)
	{
		cleanup();
		return false;
	}

	/* Init the file watcher */
	_raymarch_watcher.path = fs::current_path() / _config.assets.folder / _config.assets.raymarch_program.scene_file;
	_raymarch_watcher.interval = _config.assets.raymarch_program.scene_reload_interval;
	_raymarch_watcher.callback = [this]()
	{
		/* Print formatted time */
		std::time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::cout << std::put_time(std::localtime(&current_time), "[%T]") << " Recompiling scene..." << std::endl;

		/* Make sure a valid GL context is current in the calling thread */
		SDL_GL_MakeCurrent(_window, _compiler_context);

		_temp_program = recompile_raymarch_program();

		if (_temp_program != invalid_handle)
			_swap_program = true;
	};

	/* Scene setup */
	_camera.focal_length = 1.67f;
	_camera.position = { .0f, 2.f, 5.f };
	_camera.view = { .0f, -.5f, -1.f };
	_camera.up = { .0f, 1.f, .0f };
	_camera.right = { 1.f, .0f, .0f };

	_light.direction = { -1.f, -1.f, -1.f };
	_light.color = { 1.f, 1.f, 1.f };

	_scene.floor_height = -.3f;
	_scene.fog_color = { .6f, .7f, .8f };
	_scene.sky_color = { .8f, .9f, 1.f };

	_initialized = true;
	return true;
}

void application::cleanup()
{
	if (_raymarch_program != invalid_handle)
		glDeleteProgram(_raymarch_program);
	if (_copy_program != invalid_handle)
		glDeleteProgram(_copy_program);

	if (_offscreen_buffer != invalid_handle)
		glDeleteTextures(1, &_offscreen_buffer);
	if (_fullscreen_quad != invalid_handle)
		glDeleteVertexArrays(1, &_fullscreen_quad);

	imgui_shutdown();

	if (_render_context)
		SDL_GL_DeleteContext(_render_context);
	if (_compiler_context)
		SDL_GL_DeleteContext(_compiler_context);

	if (_window)
		SDL_DestroyWindow(_window);
	SDL_Quit();
}

void application::run()
{
	if (!_initialized)
		return;

	_raymarch_watcher.start();
	_should_run = true;
	auto start_time = hr_clock::now();

	while (_should_run)
	{
		/* Process OS Events */
		process_messages();

		/* Swap the program if it has been modified */
		swap_raymarch_program();

		_time_running = std::chrono::duration_cast<millis_interval>(hr_clock::now() - start_time);

		/* Actual rendering */
		raymarch();
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		copy_to_framebuffer();

		/* Generate the GUI */
		imgui_new_frame();
		generate_gui();
		ImGui::Render();
		
		/* Swap buffers */
		SDL_GL_SwapWindow(_window);
	}

	_raymarch_watcher.stop();
}

bool application::open_window()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		std::cout << "ERROR: Failed to initialize SDL!" << std::endl;
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR_VERSION);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

#ifdef _DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG |
						SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

	auto fullscreen_flag = _config.fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
	_window = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _config.resolution.width,
							   _config.resolution.height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | fullscreen_flag);

	if (!_window)
	{
		std::cout << "ERROR: Could not create a window!" << std::endl;
		return false;
	}
	
	return true;
}

bool application::initialize_opengl()
{
	/* Create contexts */
	_render_context = SDL_GL_CreateContext(_window);
	_compiler_context = SDL_GL_CreateContext(_window);
	SDL_GL_MakeCurrent(_window, _render_context);

	/* Load Extensions */
	gl3wInit();
	if (!gl3wIsSupported(OPENGL_MAJOR_VERSION, OPENGL_MINOR_VERSION))
	{
		std::cout << "ERROR: OpenGL " << OPENGL_MAJOR_VERSION << "." << OPENGL_MINOR_VERSION << " core not supported!" << std::endl;
		return false;
	}

	/* Basic OpenGL initialization */
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);

#ifdef _DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback((GLDEBUGPROC)gl_debug_callback, nullptr);
#endif

	if (glGetError() != GL_NO_ERROR)
	{
		std::cout << "ERROR: Failed to initialize OpenGL!" << std::endl;
		return false;
	}

	return true;
}

bool application::create_opengl_resources()
{
	/* Geometry used to issue the copy of the image onto the framebuffer */
	glGenVertexArrays(1, &_fullscreen_quad);
	glBindVertexArray(_fullscreen_quad);

	/* Image written by the compute and copied onto the framebuffer */
	glGenTextures(1, &_offscreen_buffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _offscreen_buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _config.resolution.width,
				 _config.resolution.height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindImageTexture(0, _offscreen_buffer, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	/* Raymarch program */
	_raymarch_program = recompile_raymarch_program();
	if (_raymarch_program == invalid_handle)
		return false;

	/* Copy program */
	{
		auto vs_source = get_content_of_file(_config.assets.folder / _config.assets.copy_program.vertex_shader_filename);
		uint32_t vs = compile_shader(vs_source, shader_type::VERTEX);
		assert(vs != invalid_handle);

		auto fs_source = get_content_of_file(_config.assets.folder / _config.assets.copy_program.fragment_shader_filename);
		uint32_t fs = compile_shader(fs_source, shader_type::FRAGMENT);
		assert(fs != invalid_handle);

		_copy_program = link_program({ vs, fs });

		glDeleteShader(vs);
		glDeleteShader(fs);
	
		if (_copy_program == invalid_handle)
		{
			std::cout << "ERROR: Failed to create a valid OpenGL program!" << std::endl;
			return false;
		}
	}

	if (glGetError() != GL_NO_ERROR)
	{
		std::cout << "ERROR: Failed to create OpenGL resources!" << std::endl;
		return false;
	}

	return true;
}

void application::process_messages()
{
	static SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		imgui_process_event(&event);

		switch (event.type)
		{
			case SDL_QUIT:
				_should_run = false;
				break;

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					_should_run = false;
				break;

			default:
				break;
		}
	}
}

void application::swap_raymarch_program()
{
	if (!_swap_program)
		return;

	glUseProgram(0);
	glDeleteProgram(_raymarch_program);

	_raymarch_program = _temp_program;
	_temp_program = invalid_handle;

	_swap_program = false;
}

// TODO(Corralx): We should really be using an UBO for all of these instead of doing 25+ glUniform* calls
void application::raymarch()
{
	glUseProgram(_raymarch_program);

	bind_default_uniforms();
	bind_user_uniforms();

	// TODO(Corralx): Find a better way to handle this (Maybe just precompute the values?)
	uint32_t x = static_cast<uint32_t>(std::ceil(_config.resolution.width / static_cast<float>(_config.group_size.x)));
	uint32_t y = static_cast<uint32_t>(std::ceil(_config.resolution.height / static_cast<float>(_config.group_size.y)));
	glDispatchCompute(x, y, 1);
}

void application::copy_to_framebuffer()
{
	using namespace locations;

	glUseProgram(_copy_program);

	glUniform1ui(SCREEN_WIDTH, _config.resolution.width);
	glUniform1ui(SCREEN_HEIGHT, _config.resolution.height);

	glUniform1f(VIGNETTE_RADIUS, _postprocess.vignette_radius);
	glUniform1f(VIGNETTE_SMOOTHNESS, _postprocess.vignette_smoothness);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void application::generate_gui()
{
	// NOTE(Corralx): The version of ImGui we are using is patched to support a custom sized spacing
	static float gui_space = 10.f;

	ImGui::Text("Scene info");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	if (ImGui::Button("Open Scene", ImVec2(120, 25)))
		open_scene_file();
	ImGui::Spacing(gui_space);

	/* Raymarch parameters */
	if (ImGui::CollapsingHeader("Raymarch settings"))
	{
		ImGui::Spacing(gui_space);
		ImGui::InputFloat("Epsilon", &_raymarch.epsilon, .0f, .0f, 4);
		ImGui::InputFloat("Z Far", &_raymarch.z_far, .0f, .0f, 2);
		ImGui::InputFloat("Normal epsilon", &_raymarch.normal_epsilon, .0f, .0f, 4);
		ImGui::InputFloat("Starting step", &_raymarch.starting_step, .0f, .0f, 3);
		ImGui::InputInt("Max iterations", &_raymarch.max_iterations);
		ImGui::Checkbox("Enable shadow", &_raymarch.enable_shadow);
		ImGui::Checkbox("Soft Shadow", &_raymarch.soft_shadow);
		ImGui::InputFloat("Shadow quality", &_raymarch.shadow_quality, .0f, .0f, 2);
		ImGui::InputFloat("Shadow epsilon", &_raymarch.shadow_epsilon, .0f, .0f, 4);
		ImGui::InputFloat("Shadow starting step", &_raymarch.shadow_starting_step, .0f, .0f, 3);
		ImGui::InputFloat("Shadow max step", &_raymarch.shadow_max_step, .0f, .0f, 3);
		ImGui::Checkbox("Enable ambient occlusion", &_raymarch.enable_ambient_occlusion);
		ImGui::InputFloat("Ambient occlusion step", &_raymarch.ambient_occlusion_step, .0f, .0f, 3);
		ImGui::InputInt("Ambient occlusion iterations", &_raymarch.ambient_occlusion_iterations);
		ImGui::Spacing(gui_space);
	}

	/* Scene parameters */
	if (ImGui::CollapsingHeader("Scene settings"))
	{
		ImGui::Spacing(gui_space);
		ImGui::InputFloat("Floor Height", &_scene.floor_height);
		ImGui::InputFloat3("Sky Color", glm::value_ptr(_scene.sky_color));
		ImGui::Spacing(gui_space);

		ImGui::Text("Postprocessing");
		ImGui::SliderFloat("Vignette radius", &_postprocess.vignette_radius, .5f, 1.f);
		ImGui::SliderFloat("Vignette smoothness", &_postprocess.vignette_smoothness, .0f, .5f);
		ImGui::Spacing(gui_space);

		ImGui::Text("Camera");
		ImGui::SliderFloat("Focal length", &_camera.focal_length, 1.f, 5.f);
		ImGui::InputFloat3("Position", glm::value_ptr(_camera.position));
		ImGui::InputFloat3("View", glm::value_ptr(_camera.view));
		ImGui::Spacing(gui_space);

		ImGui::Text("Light");
		ImGui::SliderFloat3("Direction", glm::value_ptr(_light.direction), -1.f, 1.f);
		ImGui::SliderFloat3("Color", glm::value_ptr(_light.color), 0.f, 1.f);
		ImGui::Spacing(gui_space);
	}

	/* User-declared parameters */
	generate_gui_for_user_uniforms();
}

uint32_t application::recompile_raymarch_program()
{
	fs::path full_assets_path = fs::current_path() / _config.assets.folder;

	auto cs_source = get_content_of_file(full_assets_path / _config.assets.raymarch_program.base_file);
	cs_source += get_content_of_file(full_assets_path / _config.assets.raymarch_program.library_file);
	cs_source += get_content_of_file(full_assets_path / _config.assets.raymarch_program.scene_file);
	cs_source += get_content_of_file(full_assets_path / _config.assets.raymarch_program.main_file);

	uint32_t cs = compile_shader(cs_source, shader_type::COMPUTE);
	assert(cs != invalid_handle);

	uint32_t program = link_program({ cs });

	glDeleteShader(cs);

	/* Extract user-declared uniforms from compute source */
	auto unis = extract_uniform(cs_source);

	/* This is _after_ the call to extract_uniform so we can use glslang error messages if something is wrong */
	if (program == invalid_handle)
	{
		std::cout << "ERROR: Failed to create a valid OpenGL program!" << std::endl;
		return program;
	}

	/* Copy user-defined values from the old uniforms to avoid resetting the value */
	copy_uniforms_value(_uniforms, unis);
	
	/* Look up the uniforms location for the binding in the actual program */
	get_uniforms_locations(unis, program);
	_uniforms = std::move(unis);

	return program;
}

void application::open_scene_file()
{
	std::string scene_path = (fs::current_path() / _config.assets.folder /
							 _config.assets.raymarch_program.scene_file).string();

#ifdef _WIN32
	ShellExecute(0, 0, scene_path.c_str(), 0, 0, SW_SHOW);
#else
	// https://stackoverflow.com/questions/6143100/how-do-i-open-a-file-in-its-default-program-linux/6143139#6143139
	auto pid = fork();
	if (pid == 0)
	{
		execl("/usr/bin/xdg-open", "xdg-open", scene_path.c_str(), (char *)0);
		exit(1);
	}
#endif
}

void application::bind_default_uniforms()
{
	using namespace locations;

	glUniform1f(EPSILON, _raymarch.epsilon);
	glUniform1f(Z_FAR, _raymarch.z_far);
	glUniform1f(NORMAL_EPSILON, _raymarch.normal_epsilon);
	glUniform1f(STARTING_STEP, _raymarch.starting_step);
	glUniform1i(MAX_ITERATIONS, _raymarch.max_iterations);

	glUniform1ui(ENABLE_SHADOW, _raymarch.enable_shadow);
	glUniform1ui(SOFT_SHADOW, _raymarch.soft_shadow);
	glUniform1f(SHADOW_QUALITY, _raymarch.shadow_quality);
	glUniform1f(SHADOW_EPSILON, _raymarch.shadow_epsilon);
	glUniform1f(SHADOW_STARTING_STEP, _raymarch.shadow_starting_step);
	glUniform1f(SHADOW_MAX_STEP, _raymarch.shadow_max_step);

	glUniform1i(ENABLE_AMBIENT_OCCLUSION, _raymarch.enable_ambient_occlusion);
	glUniform1f(AMBIENT_OCCLUSION_STEP, _raymarch.ambient_occlusion_step);
	glUniform1i(AMBIENT_OCCLUSION_ITERATIONS, _raymarch.ambient_occlusion_iterations);

	glUniform1f(TIME, _time_running.count());

	glUniform1f(FLOOR_HEIGHT, _scene.floor_height);
	glUniform3fv(SKY_COLOR, 1, glm::value_ptr(_scene.sky_color));

	glUniform3fv(LIGHT_DIRECTION, 1, glm::value_ptr(_light.direction));
	glUniform3fv(LIGHT_COLOR, 1, glm::value_ptr(_light.color));

	glUniform3fv(CAMERA_POSITION, 1, glm::value_ptr(_camera.position));
	glUniform3fv(CAMERA_VIEW, 1, glm::value_ptr(_camera.view));
	glUniform3fv(CAMERA_UP, 1, glm::value_ptr(_camera.up));
	glUniform3fv(CAMERA_RIGHT, 1, glm::value_ptr(_camera.right));
	glUniform1f(FOCAL_LENGTH, _camera.focal_length);

	glUniform1ui(SCREEN_WIDTH, _config.resolution.width);
	glUniform1ui(SCREEN_HEIGHT, _config.resolution.height);
}

void application::bind_user_uniforms()
{
	for (const auto& u : _uniforms)
	{
		if (u.location == invalid_handle)
			continue;

		switch (u.type)
		{
			case uniform_type::FLOAT:
				glUniform1f(u.location, u.vec4.x);
				break;

			case uniform_type::VEC2:
				glUniform2fv(u.location, 1, glm::value_ptr<float>(u.vec4));
				break;

			case uniform_type::VEC3:
				glUniform3fv(u.location, 1, glm::value_ptr<float>(u.vec4));
				break;

			case uniform_type::VEC4:
				glUniform4fv(u.location, 1, glm::value_ptr<float>(u.vec4));
				break;

			case uniform_type::DOUBLE:
				glUniform1d(u.location, static_cast<double>(u.vec4.x));
				break;

			case uniform_type::DVEC2:
				glUniform2d(u.location, static_cast<double>(u.vec4.x),
							static_cast<double>(u.vec4.y));
				break;

			case uniform_type::DVEC3:
				glUniform3d(u.location, static_cast<double>(u.vec4.x),
							static_cast<double>(u.vec4.y),
							static_cast<double>(u.vec4.z));
				break;

			case uniform_type::DVEC4:
				glUniform4d(u.location, static_cast<double>(u.vec4.x),
							static_cast<double>(u.vec4.y),
							static_cast<double>(u.vec4.z),
							static_cast<double>(u.vec4.w));
				break;

			case uniform_type::INT:
				glUniform1i(u.location, static_cast<int32_t>(u.ivec4.x));
				break;

			case uniform_type::IVEC2:
				glUniform2iv(u.location, 1, glm::value_ptr<int32_t>(u.ivec4));
				break;

			case uniform_type::IVEC3:
				glUniform3iv(u.location, 1, glm::value_ptr<int32_t>(u.ivec4));
				break;

			case uniform_type::IVEC4:
				glUniform4iv(u.location, 1, glm::value_ptr<int32_t>(u.ivec4));
				break;

			case uniform_type::UINT:
				glUniform1ui(u.location, static_cast<uint32_t>(u.ivec4.x));
				break;

			case uniform_type::UVEC2:
				glUniform2ui(u.location, static_cast<uint32_t>(u.ivec4.x),
							 static_cast<uint32_t>(u.ivec4.y));
				break;

			case uniform_type::UVEC3:
				glUniform3ui(u.location, static_cast<uint32_t>(u.ivec4.x),
							 static_cast<uint32_t>(u.ivec4.y),
							 static_cast<uint32_t>(u.ivec4.z));
				break;

			case uniform_type::UVEC4:
				glUniform4ui(u.location, static_cast<uint32_t>(u.ivec4.x),
							 static_cast<uint32_t>(u.ivec4.y),
							 static_cast<uint32_t>(u.ivec4.z),
							 static_cast<uint32_t>(u.ivec4.w));

			case uniform_type::BOOL:
				glUniform1ui(u.location, static_cast<uint32_t>(u.boolean));
				break;

			default:
				assert(false && "Unsupported uniform type requested!");
				break;
		}
	}
}

void application::generate_gui_for_user_uniforms()
{
	if (_uniforms.empty() || !ImGui::CollapsingHeader("Custom parameters", nullptr, true, true))
		return;

	for (auto& u : _uniforms)
	{
		switch (u.type)
		{
			case uniform_type::FLOAT:
			case uniform_type::DOUBLE:
				ImGui::InputFloat(u.name.c_str(), glm::value_ptr<float>(u.vec4));
				break;

			case uniform_type::VEC2:
			case uniform_type::DVEC2:
				ImGui::InputFloat2(u.name.c_str(), glm::value_ptr<float>(u.vec4));
				break;

			case uniform_type::VEC3:
			case uniform_type::DVEC3:
				ImGui::InputFloat3(u.name.c_str(), glm::value_ptr<float>(u.vec4));
				break;

			case uniform_type::VEC4:
			case uniform_type::DVEC4:
				ImGui::InputFloat4(u.name.c_str(), glm::value_ptr<float>(u.vec4));
				break;

			case uniform_type::INT:
			case uniform_type::UINT:
				ImGui::InputInt(u.name.c_str(), glm::value_ptr<int32_t>(u.ivec4));
				break;

			case uniform_type::IVEC2:
			case uniform_type::UVEC2:
				ImGui::InputInt2(u.name.c_str(), glm::value_ptr<int32_t>(u.ivec4));
				break;

			case uniform_type::IVEC3:
			case uniform_type::UVEC3:
				ImGui::InputInt3(u.name.c_str(), glm::value_ptr<int32_t>(u.ivec4));
				break;

			case uniform_type::IVEC4:
			case uniform_type::UVEC4:
				ImGui::InputInt4(u.name.c_str(), glm::value_ptr<int32_t>(u.ivec4));
				break;

			case uniform_type::BOOL:
				ImGui::Checkbox(u.name.c_str(), &u.boolean);
				break;

			default:
				assert(false);
				break;
		}
	}
}
