#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdint>
#include <chrono>
using hr_clock = std::chrono::high_resolution_clock;
#include <ctime>
#include <iomanip>
#include <experimental/filesystem>

#ifdef _WIN32
#include "Windows.h"
#include "Shellapi.h"
#else
#include <unistd.h>
#endif

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GL/gl3w.h"
#include "SDL2/SDL.h"
#include "imgui/imgui.h"
#include "imgui_sdl_bridge.hpp"
#include "file_watcher.hpp"
#include "common.hpp"

struct compute_group_size
{
	uint32_t x;
	uint32_t y;
};

struct raymarch_program_info
{
	fs::path scene_path;

	std::string base_source;
	std::string library_source;
	std::string main_source;
};

struct raymarch_t
{
	/* Base */
	float epsilon = .001f;
	float z_far = 20.f;
	float normal_epsilon = .0001f;
	float starting_step = 1.f;
	int32_t max_iterations = 150;

	/* Shadows */
	bool enable_shadow = true;
	bool soft_shadow = true;
	float shadow_quality = 64.f;
	float shadow_epsilon = .001f;
	float shadow_starting_step = .03f;
	float shadow_max_step = 7.f;

	/* Ambient Occlusion */
	bool enable_ambient_occlusion = true;
	float ambient_occlusion_step = .01f;
	int32_t ambient_occlusion_iterations = 5;
};

struct camera_t
{
	glm::vec3 position;
	glm::vec3 view;
	glm::vec3 up;
	glm::vec3 right;
	float focal_length;
};

struct light_t
{
	glm::vec3 direction;
	glm::vec3 color;
};

static SDL_Window* open_window();
static void copy_uniforms_value(std::vector<uniform_t>& old_uniforms, std::vector<uniform_t>& new_uniforms);
static void get_locations(std::vector<uniform_t>& uniforms, uint32_t program);
static void generate_gui(std::vector<uniform_t>& uniforms);
static void bind_uniforms(const std::vector<uniform_t>& uniforms);

static constexpr uint32_t WINDOW_WIDTH = 1280;
static constexpr uint32_t WINDOW_HEIGHT = 720;
static constexpr char* WINDOW_NAME = "Helios";
static constexpr bool WINDOW_FULLSCREEN = false;
static constexpr uint32_t OPENGL_MAJOR_VERSION = 4;
static constexpr uint32_t OPENGL_MINOR_VERSION = 3;
static constexpr char* ASSETS_FOLDER = "resources";
static constexpr auto shader_reload_interval = 500ms;

int main(int, char*[])
{
	auto sdl_window = open_window();
	auto render_context = SDL_GL_CreateContext(sdl_window);
	auto compiler_context = SDL_GL_CreateContext(sdl_window);
	SDL_GL_MakeCurrent(sdl_window, render_context);

	gl3wInit();
	if (!gl3wIsSupported(OPENGL_MAJOR_VERSION, OPENGL_MINOR_VERSION))
	{
		std::cout << "Opengl " << OPENGL_MAJOR_VERSION << "." << OPENGL_MINOR_VERSION << " core not supported!";
		return -1;
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

	/* Geometry used to issue the copy of the image onto the framebuffer */
	uint32_t quad_vao;
	glGenVertexArrays(1, &quad_vao);
	glBindVertexArray(quad_vao);

	/* Image written by the compute and copied onto the framebuffer */
	GLuint offscreen_texture;
	glGenTextures(1, &offscreen_texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, offscreen_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindImageTexture(0, offscreen_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	assert(glGetError() == GL_NO_ERROR);

	// This is used to avoid setting an uniform we don't use and cause and INVALID_OPERATION
	bool use_time = false;

	/* Compile the programs*/
	raymarch_program_info rpi{};
	{
		rpi.scene_path = fs::path(ASSETS_FOLDER) / "raymarch_scene.comp";

		rpi.base_source = get_content_of_file(fs::path(ASSETS_FOLDER) / "raymarch_base.comp");
		rpi.library_source = get_content_of_file(fs::path(ASSETS_FOLDER) / "raymarch_library.comp");
		rpi.main_source = get_content_of_file(fs::path(ASSETS_FOLDER) / "raymarch_main.comp");
	}

	std::vector<uniform_t> uniforms{};
	uint32_t raymarch_program = invalid_handle;
	{
		std::string raymarch_source = rpi.base_source + rpi.library_source +
									  get_content_of_file(rpi.scene_path) + rpi.main_source;
		uint32_t raymarch_shader = compile_shader(raymarch_source, shader_type::COMPUTE);

		raymarch_program = link_program({ raymarch_shader });
		glDeleteShader(raymarch_shader);

		auto new_uniforms = extract_uniform(raymarch_source);
		copy_uniforms_value(uniforms, new_uniforms);
		get_locations(new_uniforms, raymarch_program);
		uniforms = std::move(new_uniforms);

		use_time = (glGetUniformLocation(raymarch_program, "time") != invalid_handle);
	}
	assert(raymarch_program != invalid_handle);

	uint32_t copy_program = invalid_handle;
	{
		auto base_shader_source = get_content_of_file(fs::path(ASSETS_FOLDER) / "base_vertex.vert");
		uint32_t base_shader = compile_shader(base_shader_source, shader_type::VERTEX);

		auto copy_shader_source = get_content_of_file(fs::path(ASSETS_FOLDER) / "copy.frag");
		uint32_t copy_shader = compile_shader(copy_shader_source, shader_type::FRAGMENT);
		
		copy_program = link_program({ base_shader, copy_shader });
		glDeleteShader(base_shader);
		glDeleteShader(copy_shader);
	}
	assert(copy_program != invalid_handle);

	uint32_t compiled_program = invalid_handle;
	bool swap_program = false;

	/* Launch the file watcher in background to dynamically recompile the scene */
	file_watcher fw(rpi.scene_path, shader_reload_interval,
	[sdl_window, compiler_context, &swap_program, &compiled_program, &rpi, &uniforms, &use_time]()
	{
		std::time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::cout << std::put_time(std::localtime(&current_time), "[%T]") << " Recompiling scene..." << std::endl;

		SDL_GL_MakeCurrent(sdl_window, compiler_context);
		std::string compute_source = rpi.base_source + rpi.library_source +
									 get_content_of_file(rpi.scene_path) + rpi.main_source;
		uint32_t compute_shader = compile_shader(compute_source, shader_type::COMPUTE);

		// TODO(Corralx): Emit an error to the user if the compilation has failed
		compiled_program = link_program({ compute_shader });
		glDeleteShader(compute_shader);

		auto new_uniforms = extract_uniform(compute_source);
		copy_uniforms_value(uniforms, new_uniforms);
		get_locations(new_uniforms, compiled_program);
		uniforms = std::move(new_uniforms);

		use_time = (glGetUniformLocation(compiled_program, "time") != invalid_handle);

		swap_program = true;
		assert(compiled_program != invalid_handle);
	});
	fw.start();

	/* Initialize the default uniforms */
	raymarch_t raymarch_params{};

	camera_t camera{};
	camera.focal_length = 1.67f;
	camera.position = { .0f, 2.f, 5.f };
	camera.view = { .0f, -.5f, -1.f };
	camera.up = { .0f, 1.f, .0f };
	camera.right = { 1.f, .0f, .0f };

	light_t light{};
	light.direction = { -1.f, -1.f, -1.f };
	light.color = { 1.f, 1.f, 1.f };

	float vignette_radius = .9f;
	float vignette_smoothness = .07f;

	imgui_init(sdl_window);
	float gui_space = 10.f;

	/* Render loop */
	bool should_quit = false;
	SDL_Event event;
	std::chrono::duration<float> time_passed;
	auto start_time = hr_clock::now();

	compute_group_size group_size =
	{
		static_cast<uint32_t>(std::ceil(WINDOW_WIDTH / 32.f)),
		static_cast<uint32_t>(std::ceil(WINDOW_HEIGHT / 32.f))
	};

	while (!should_quit)
	{
		/* Message Queue */
		while (SDL_PollEvent(&event))
		{
			imgui_process_event(&event);

			switch (event.type)
			{
				case SDL_QUIT:
					should_quit = true;
					break;

				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE)
						should_quit = true;
					break;

				default:
					break;
			}
		}

		/* Use the new program if the user changed the scene file */
		if (swap_program)
		{
			glUseProgram(0);
			glDeleteProgram(raymarch_program);
			raymarch_program = compiled_program;
			compiled_program = invalid_handle;
			swap_program = false;
		}

		imgui_new_frame();

		/* Dispatch the compute to generate the image */
		glUseProgram(raymarch_program);

		// TODO(Corralx): an UBO should be used for performance here
		/* Default uniforms */
		glUniform1f(1000, raymarch_params.epsilon);
		glUniform1f(1001, raymarch_params.z_far);
		glUniform1f(1002, raymarch_params.normal_epsilon);
		glUniform1f(1003, raymarch_params.starting_step);
		glUniform1i(1004, raymarch_params.max_iterations);
		glUniform1ui(1005, raymarch_params.enable_shadow);
		glUniform1ui(1006, raymarch_params.soft_shadow);
		glUniform1f(1007, raymarch_params.shadow_quality);
		glUniform1f(1008, raymarch_params.shadow_epsilon);
		glUniform1f(1009, raymarch_params.shadow_starting_step);
		glUniform1f(1010, raymarch_params.shadow_max_step);
		glUniform1i(1011, raymarch_params.enable_ambient_occlusion);
		glUniform1f(1012, raymarch_params.ambient_occlusion_step);
		glUniform1i(1013, raymarch_params.ambient_occlusion_iterations);
		
		if (use_time)
		{
			time_passed = std::chrono::duration_cast<std::chrono::duration<float>>(hr_clock::now() - start_time);
			glUniform1f(1014, time_passed.count());
		}

		glUniform3fv(1015, 1, glm::value_ptr(light.direction));
		glUniform3fv(1016, 1, glm::value_ptr(light.color));
		glUniform3fv(1017, 1, glm::value_ptr(camera.position));
		glUniform3fv(1018, 1, glm::value_ptr(camera.view));
		glUniform3fv(1019, 1, glm::value_ptr(camera.up));
		glUniform3fv(1020, 1, glm::value_ptr(camera.right));
		glUniform1f(1021, camera.focal_length);
		glUniform1ui(1022, WINDOW_WIDTH);
		glUniform1ui(1023, WINDOW_HEIGHT);

		/* User uniforms */
		bind_uniforms(uniforms);

		glDispatchCompute(group_size.x, group_size.y, 1);

		/* Make sure the compute has finished before copying the content */
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		
		/* Copy the content of the image onto the framebuffer */
		glUseProgram(copy_program);
		glUniform1ui(0, WINDOW_WIDTH);
		glUniform1ui(1, WINDOW_HEIGHT);
		glUniform1f(2, vignette_radius);
		glUniform1f(3, vignette_smoothness);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		/* ImGui */
		{
			ImGui::Text("Scene info");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			/* Open the scene file with the default associated program */
			if (ImGui::Button("Open Scene", ImVec2(120, 25)))
			{
#ifdef _WIN32
				ShellExecuteW(0, 0, rpi.scene_path.c_str(), 0, 0, SW_SHOW);
#else
				// https://stackoverflow.com/questions/6143100/how-do-i-open-a-file-in-its-default-program-linux/6143139#6143139
				auto pid = fork();
				if (pid == 0)
				{
					execl("/usr/bin/xdg-open", "xdg-open", rpi.scene_path.c_str(), (char *)0);
					exit(1);
				}
#endif
			}
			ImGui::Spacing(gui_space);

			if (ImGui::CollapsingHeader("Raymarch settings"))
			{
				ImGui::InputFloat("Epsilon", &raymarch_params.epsilon, .0f, .0f, 4);
				ImGui::InputFloat("Z Far", &raymarch_params.z_far, .0f, .0f, 2);
				ImGui::InputFloat("Normal epsilon", &raymarch_params.normal_epsilon, .0f, .0f, 4);
				ImGui::InputFloat("Starting step", &raymarch_params.starting_step, .0f, .0f, 3);
				ImGui::InputInt("Max iterations", &raymarch_params.max_iterations);
				ImGui::Checkbox("Enable shadow", &raymarch_params.enable_shadow);
				ImGui::Checkbox("Soft Shadow", &raymarch_params.soft_shadow);
				ImGui::InputFloat("Shadow quality", &raymarch_params.shadow_quality, .0f, .0f, 2);
				ImGui::InputFloat("Shadow epsilon", &raymarch_params.shadow_epsilon, .0f, .0f, 4);
				ImGui::InputFloat("Shadow starting step", &raymarch_params.shadow_starting_step, .0f, .0f, 3);
				ImGui::InputFloat("Shadow max step", &raymarch_params.shadow_max_step, .0f, .0f, 3);
				ImGui::Checkbox("Enable ambient occlusion", &raymarch_params.enable_ambient_occlusion);
				ImGui::InputFloat("Ambient occlusion step", &raymarch_params.ambient_occlusion_step, .0f, .0f, 3);
				ImGui::InputInt("Ambient occlusion iterations", &raymarch_params.ambient_occlusion_iterations);
				ImGui::Spacing(gui_space);
			}

			if (ImGui::CollapsingHeader("Scene settings"))
			{
				ImGui::Text("Postprocessing");
				ImGui::SliderFloat("Vignette radius", &vignette_radius, .5f, 1.f);
				ImGui::SliderFloat("Vignette smoothness", &vignette_smoothness, .0f, .5f);

				ImGui::Text("Camera");
				ImGui::SliderFloat("Focal length", &camera.focal_length, 1.f, 5.f);
				ImGui::InputFloat3("Position", glm::value_ptr(camera.position));
				ImGui::InputFloat3("View", glm::value_ptr(camera.view));
				ImGui::Spacing(gui_space);

				ImGui::Text("Light");
				ImGui::SliderFloat3("Direction", glm::value_ptr(light.direction), -1.f, 1.f);
				ImGui::SliderFloat3("Color", glm::value_ptr(light.color), 0.f, 1.f);
				ImGui::Spacing(gui_space);
			}

			generate_gui(uniforms);
		}

		ImGui::Render();
		SDL_GL_SwapWindow(sdl_window);
	}

	/* Application cleanup */
	fw.stop();
	glDeleteProgram(raymarch_program);
	glDeleteProgram(copy_program);

	glDeleteTextures(1, &offscreen_texture);
	glDeleteVertexArrays(1, &quad_vao);

	imgui_shutdown();
	SDL_GL_DeleteContext(render_context);
	SDL_GL_DeleteContext(compiler_context);
	SDL_DestroyWindow(sdl_window);
	SDL_Quit();

	return 0;
}

SDL_Window* open_window()
{
	SDL_Init(SDL_INIT_VIDEO);
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

	auto fullscreen_flag = WINDOW_FULLSCREEN ? SDL_WINDOW_FULLSCREEN : 0;
	return SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | fullscreen_flag);
}

static void get_locations(std::vector<uniform_t>& uniforms, uint32_t program)
{
	for (auto& u : uniforms)
	{
		uint32_t loc = glGetUniformLocation(program, u.name.c_str());

		// This should not happen because glslang optimize away the unused uniforms too
		if (loc == invalid_handle)
			std::cout << "Could not retrieve location for uniform \"" << u.name
					  << "\" (Maybe it was optimized away?)" << std::endl;

		u.location = loc;
	}
}

static void generate_gui(std::vector<uniform_t>& uniforms)
{
	if (uniforms.empty())
		return;

	if (!ImGui::CollapsingHeader("Custom uniforms"))
		return;
	
	for (auto& u : uniforms)
	{
		switch (u.type)
		{
			case uniform_type::FLOAT:
			case uniform_type::DOUBLE:
				ImGui::InputFloat(u.name.c_str(), glm::value_ptr<float>(u.float4));
				break;

			case uniform_type::VEC2:
			case uniform_type::DVEC2:
				ImGui::InputFloat2(u.name.c_str(), glm::value_ptr<float>(u.float4));
				break;

			case uniform_type::VEC3:
			case uniform_type::DVEC3:
				ImGui::InputFloat3(u.name.c_str(), glm::value_ptr<float>(u.float4));
				break;

			case uniform_type::VEC4:
			case uniform_type::DVEC4:
				ImGui::InputFloat4(u.name.c_str(), glm::value_ptr<float>(u.float4));
				break;

			case uniform_type::INT:
			case uniform_type::UINT:
				ImGui::InputInt(u.name.c_str(), glm::value_ptr<int32_t>(u.int4));
				break;

			case uniform_type::IVEC2:
			case uniform_type::UVEC2:
				ImGui::InputInt2(u.name.c_str(), glm::value_ptr<int32_t>(u.int4));
				break;

			case uniform_type::IVEC3:
			case uniform_type::UVEC3:
				ImGui::InputInt3(u.name.c_str(), glm::value_ptr<int32_t>(u.int4));
				break;

			case uniform_type::IVEC4:
			case uniform_type::UVEC4:
				ImGui::InputInt4(u.name.c_str(), glm::value_ptr<int32_t>(u.int4));
				break;

			case uniform_type::BOOL:
				ImGui::Checkbox(u.name.c_str(), glm::value_ptr<bool>(u.bool4));
				break;

			default:
				assert(false);
				break;
		}
	}
}

static void bind_uniforms(const std::vector<uniform_t>& uniforms)
{
	for (const auto& u : uniforms)
	{
		if (u.location == invalid_handle)
			continue;

		switch (u.type)
		{
			case uniform_type::FLOAT:
				glUniform1f(u.location, u.float4.x);
				break;

			case uniform_type::VEC2:
				glUniform2fv(u.location, 1, glm::value_ptr<float>(u.float4));
				break;

			case uniform_type::VEC3:
				glUniform3fv(u.location, 1, glm::value_ptr<float>(u.float4));
				break;

			case uniform_type::VEC4:
				glUniform4fv(u.location, 1, glm::value_ptr<float>(u.float4));
				break;

			case uniform_type::DOUBLE:
				glUniform1d(u.location, static_cast<double>(u.float4.x));
				break;

			case uniform_type::DVEC2:
				glUniform2d(u.location, static_cast<double>(u.float4.x),
										static_cast<double>(u.float4.y));
				break;

			case uniform_type::DVEC3:
				glUniform3d(u.location, static_cast<double>(u.float4.x),
										static_cast<double>(u.float4.y),
										static_cast<double>(u.float4.z));
				break;

			case uniform_type::DVEC4:
				glUniform4d(u.location, static_cast<double>(u.float4.x),
										static_cast<double>(u.float4.y),
										static_cast<double>(u.float4.z),
										static_cast<double>(u.float4.w));
				break;

			case uniform_type::INT:
				glUniform1i(u.location, static_cast<int32_t>(u.int4.x));
				break;

			case uniform_type::IVEC2:
				glUniform2iv(u.location, 1, glm::value_ptr<int32_t>(u.int4));
				break;

			case uniform_type::IVEC3:
				glUniform3iv(u.location, 1, glm::value_ptr<int32_t>(u.int4));
				break;

			case uniform_type::IVEC4:
				glUniform4iv(u.location, 1, glm::value_ptr<int32_t>(u.int4));
				break;

			case uniform_type::UINT:
				glUniform1ui(u.location, static_cast<uint32_t>(u.int4.x));
				break;

			case uniform_type::UVEC2:
				glUniform2ui(u.location, static_cast<uint32_t>(u.int4.x),
										 static_cast<uint32_t>(u.int4.y));
				break;

			case uniform_type::UVEC3:
				glUniform3ui(u.location, static_cast<uint32_t>(u.int4.x),
										 static_cast<uint32_t>(u.int4.y),
										 static_cast<uint32_t>(u.int4.z));
				break;

			case uniform_type::UVEC4:
				glUniform4ui(u.location, static_cast<uint32_t>(u.int4.x),
										 static_cast<uint32_t>(u.int4.y),
										 static_cast<uint32_t>(u.int4.z),
										 static_cast<uint32_t>(u.int4.w));

			case uniform_type::BOOL:
				glUniform1ui(u.location, static_cast<uint32_t>(u.bool4.x));
				break;

			case uniform_type::BVEC2:
				glUniform2ui(u.location, static_cast<uint32_t>(u.bool4.x),
										 static_cast<uint32_t>(u.bool4.y));
				break;

			case uniform_type::BVEC3:
				glUniform3ui(u.location, static_cast<uint32_t>(u.bool4.x),
										 static_cast<uint32_t>(u.bool4.y),
										 static_cast<uint32_t>(u.bool4.z));
				break;

			case uniform_type::BVEC4:
				glUniform4ui(u.location, static_cast<uint32_t>(u.bool4.x),
										 static_cast<uint32_t>(u.bool4.y),
										 static_cast<uint32_t>(u.bool4.z),
										 static_cast<uint32_t>(u.bool4.w));
				break;

			default:
				assert(false);
				break;
		}
	}
}

static void copy_uniforms_value(std::vector<uniform_t>& old_uniforms, std::vector<uniform_t>& new_uniforms)
{
	for (auto& u : new_uniforms)
	{
		// We reuse the current value only if the uniform as the same name and the same type
		auto it = std::find_if(old_uniforms.begin(), old_uniforms.end(), [&u](const uniform_t& old_u)
		{
			return u.name == old_u.name && u.type == old_u.type;
		});

		// We haven't found the uniform, so it is new and will be initialized to 0
		if (it == old_uniforms.end())
			continue;

		switch (u.type)
		{
			case uniform_type::FLOAT:
			case uniform_type::VEC2:
			case uniform_type::VEC3:
			case uniform_type::VEC4:
			case uniform_type::DOUBLE:
			case uniform_type::DVEC2:
			case uniform_type::DVEC3:
			case uniform_type::DVEC4:
				u.float4 = it->float4;
				break;

			case uniform_type::INT:
			case uniform_type::IVEC2:
			case uniform_type::IVEC3:
			case uniform_type::IVEC4:
			case uniform_type::UINT:
			case uniform_type::UVEC2:
			case uniform_type::UVEC3:
			case uniform_type::UVEC4:
				u.int4 = it->int4;
				break;

			case uniform_type::BOOL:
			case uniform_type::BVEC2:
			case uniform_type::BVEC3:
			case uniform_type::BVEC4:
				u.bool4 = it->bool4;
				break;

			default:
				assert(false);
				break;
		}
	}
}
