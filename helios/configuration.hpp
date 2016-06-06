#pragma once

#include <cstdint>
#include "common.hpp"

// NOTE(Corralx): Default values are used if no config file is found or the key is not defined
struct config_t
{
	struct
	{
		uint32_t width = 1280;
		uint32_t height = 720;
	} resolution;

	bool fullscreen = false;

	struct
	{
		uint32_t x = 32;
		uint32_t y = 32;
	} group_size;

	struct
	{
		fs::path folder = "resources";

		struct
		{
			fs::path vertex_shader_filename = "base_vertex.vert";
			fs::path fragment_shader_filename = "copy.frag";
		} copy_program;

		struct
		{
			fs::path base_file = "raymrach_base.comp";
			fs::path library_file = "raymarch_library.comp";
			fs::path scene_file = "raymarch_scene.comp";
			fs::path main_file = "raymarch_main.comp";
			std::chrono::milliseconds scene_reload_interval = 500ms;
		} raymarch_program;
	} assets;

};

config_t load_config();
