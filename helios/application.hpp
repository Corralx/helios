#pragma once

#include "configuration.hpp"
#include "file_watcher.hpp"
#include "uniform_utils.hpp"
#include "common.hpp"

#include <cstdint>
#include <chrono>
using millis_interval = std::chrono::duration<float>;

class application
{
public:
	application();
	application(const application&) = delete;
	application(application&&) = delete;
	~application() = default;

	application& operator=(const application&) = delete;
	application& operator=(const application&&) = delete;

	bool init();
	void run();
	void cleanup();

private:
	config_t _config;

	SDL_Window* _window;
	SDL_GLContext _render_context;
	SDL_GLContext _compiler_context;

	uint32_t _fullscreen_quad;
	uint32_t _offscreen_buffer;

	uint32_t _raymarch_program;
	uint32_t _copy_program; 
	std::vector<uniform_t> _uniforms;

	bool _should_run;
	bool _initialized;
	bool _render_gui;
	
	file_watcher _raymarch_watcher;
	uint32_t _temp_program;
	bool _swap_program;

	raymarch_t _raymarch;
	camera_t _camera;
	light_t _light;
	scene_t _scene;
	postprocess_t _postprocess;

	millis_interval _time_running;

	bool open_window();
	bool initialize_opengl();
	bool create_opengl_resources();

	void process_messages();
	void swap_raymarch_program();
	void raymarch();
	void copy_to_framebuffer();
	void generate_gui();

	uint32_t recompile_raymarch_program();

	void open_scene_file();

	void bind_default_uniforms();
	void bind_user_uniforms();
	void generate_gui_for_user_uniforms();
};
