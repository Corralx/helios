#include "configuration.hpp"
#include "common.hpp"

#include <iostream>
#pragma warning(push, 0)
#include "rapidjson/document.h"
#pragma warning(pop)

// Hardcoded configuration path
static constexpr char* CONFIG_PATH = "resources/config.json";

static constexpr char* RESOLUTION_KEY = "resolution";
static constexpr char* WIDTH_KEY = "width";
static constexpr char* HEIGHT_KEY = "height";
static constexpr char* FULLSCREEN_KEY = "fullscreen";
static constexpr char* GROUP_SIZE_KEY = "group_size";
static constexpr char* X_KEY = "x";
static constexpr char* Y_KEY = "y";
static constexpr char* ASSETS_KEY = "assets";
static constexpr char* FOLDER_KEY = "folder";
static constexpr char* COPY_PROGRAM_KEY = "copy_program";
static constexpr char* VERTEX_SHADER_FILENAME_KEY = "vertex_shader_filename";
static constexpr char* FRAGMENT_SHADER_FILENAME_KEY = "fragment_shader_filename";
static constexpr char* RAYMARCH_PROGRAM_KEY = "raymarch_program";
static constexpr char* BASE_FILE_KEY = "base_file";
static constexpr char* LIBRARY_FILE_KEY = "library_file";
static constexpr char* SCENE_FILE_KEY = "scene_file";
static constexpr char* MAIN_FILE_KEY = "main_file";
static constexpr char* SCENE_RELOAD_INTERVAL = "scene_reload_interval";

#define LOAD_BOOL_IF(member, doc, key) \
if (doc.HasMember(key)) \
	member = doc[key].GetBool()

#define LOAD_UINT_IF(member, doc, key) \
if (doc.HasMember(key)) \
	member = doc[key].GetUint()

#define LOAD_PATH_IF(member, doc, key) \
if (doc.HasMember(key)) \
	member = fs::path(doc[key].GetString())

config_t load_config()
{
	config_t config{};

	fs::path config_path = fs::current_path() / CONFIG_PATH;
	if (!fs::exists(config_path))
	{
		std::cout << "WARNING: The configuration could not be found!" << std::endl;
		return config;
	}

	rapidjson::Document doc;
	std::string config_content = get_content_of_file(config_path);
	doc.Parse(config_content.c_str());

	if (doc.HasParseError())
	{
		std::cout << "ERROR: Malformed configuration file!" << std::endl;
		return config;
	}

	if (doc.HasMember(RESOLUTION_KEY))
	{
		auto& resolution = doc[RESOLUTION_KEY];

		LOAD_UINT_IF(config.resolution.width, resolution, WIDTH_KEY);
		LOAD_UINT_IF(config.resolution.height, resolution, HEIGHT_KEY);
	}

	LOAD_BOOL_IF(config.fullscreen, doc, FULLSCREEN_KEY);

	if (doc.HasMember(GROUP_SIZE_KEY))
	{
		auto& group_size = doc[GROUP_SIZE_KEY];

		LOAD_UINT_IF(config.group_size.x, group_size, X_KEY);
		LOAD_UINT_IF(config.group_size.y, group_size, Y_KEY);
	}

	if (doc.HasMember(ASSETS_KEY))
	{
		auto& assets = doc[ASSETS_KEY];

		LOAD_PATH_IF(config.assets.folder, assets, FOLDER_KEY);

		if (assets.HasMember(COPY_PROGRAM_KEY))
		{
			auto& copy_program = assets[COPY_PROGRAM_KEY];

			auto& vsf = config.assets.copy_program.vertex_shader_filename;
			LOAD_PATH_IF(vsf, copy_program, VERTEX_SHADER_FILENAME_KEY);

			auto& fsf = config.assets.copy_program.fragment_shader_filename;
			LOAD_PATH_IF(fsf, copy_program, FRAGMENT_SHADER_FILENAME_KEY);
		}

		if (assets.HasMember(RAYMARCH_PROGRAM_KEY))
		{
			auto& raymarch_program = assets[RAYMARCH_PROGRAM_KEY];

			auto& bf = config.assets.raymarch_program.base_file;
			LOAD_PATH_IF(bf, raymarch_program, BASE_FILE_KEY);

			auto& lf = config.assets.raymarch_program.library_file;
			LOAD_PATH_IF(lf, raymarch_program, LIBRARY_FILE_KEY);

			auto& sf = config.assets.raymarch_program.scene_file;
			LOAD_PATH_IF(sf, raymarch_program, SCENE_FILE_KEY);

			auto& mf = config.assets.raymarch_program.main_file;
			LOAD_PATH_IF(mf, raymarch_program, MAIN_FILE_KEY);

			auto& interval = config.assets.raymarch_program.scene_reload_interval;
			if (raymarch_program.HasMember(SCENE_RELOAD_INTERVAL))
				interval = std::chrono::milliseconds(raymarch_program[SCENE_RELOAD_INTERVAL].GetUint());
		}
	}

	return config;
}

#undef LOAD_BOOL_IF
#undef LOAD_UINT_IF
#undef LOAD_PATH_IF
