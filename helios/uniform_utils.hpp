#pragma once

#include "common.hpp"
#include "uniforms_locations.hpp"

#include <cstdint>
#include <vector>
#include <string>

struct raymarch_t
{
	/* Base */
	float epsilon = .001f;
	float z_far = 30.f;
	float normal_epsilon = .0001f;
	float starting_step = 1.f;
	int32_t max_iterations = 100;

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

struct postprocess_t
{
	float vignette_radius = .9f;
	float vignette_smoothness = .07f;
};

struct scene_t
{
	glm::vec3 sky_color;
	glm::vec3 fog_color;
	float floor_height;
};

enum class uniform_type : uint8_t
{
	FLOAT = 0,
	VEC2,
	VEC3,
	VEC4,

	BOOL,

	INT,
	IVEC2,
	IVEC3,
	IVEC4,

	UINT,
	UVEC2,
	UVEC3,
	UVEC4,

	DOUBLE,
	DVEC2,
	DVEC3,
	DVEC4
};

// TODO(Corralx): Find a way to specify a default value?
// Uniform initialization syntax seems to be bugged in a lot of implementations!
struct uniform_t
{
	uniform_t(const std::string& n, uniform_type t) :
		name(n), location(invalid_location), type(t), ivec4(0) {}

	std::string name;
	int32_t location;
	uniform_type type;

	// NOTE(Corralx): ImGui does not support doubles or unsigned ints
	union
	{
		glm::vec4 vec4;
		glm::ivec4 ivec4;
		bool boolean;
	};
};

// NOTE(Corralx): After the call, the locations of the uniforms in the real compiled program are still to be retrieved
std::vector<uniform_t> extract_uniform(const std::string& source);

void copy_uniforms_value(std::vector<uniform_t>& old_uniforms, std::vector<uniform_t>& new_uniforms);
void get_uniforms_locations(std::vector<uniform_t>& uniforms, uint32_t program);
