#pragma once

#include <cstdint>

// NOTE(Corralx): We are using a custom namespace to hide the names and avoid pollution
namespace locations
{

constexpr uint32_t EPSILON						= 998;
constexpr uint32_t Z_FAR						= 999;
constexpr uint32_t NORMAL_EPSILON				= 1000;
constexpr uint32_t STARTING_STEP				= 1001;
constexpr uint32_t MAX_ITERATIONS				= 1002;

constexpr uint32_t ENABLE_SHADOW				= 1003;
constexpr uint32_t SOFT_SHADOW					= 1004;
constexpr uint32_t SHADOW_QUALITY				= 1005;
constexpr uint32_t SHADOW_EPSILON				= 1006;
constexpr uint32_t SHADOW_STARTING_STEP			= 1007;
constexpr uint32_t SHADOW_MAX_STEP				= 1008;

constexpr uint32_t ENABLE_AMBIENT_OCCLUSION		= 1009;
constexpr uint32_t AMBIENT_OCCLUSION_STEP		= 1010;
constexpr uint32_t AMBIENT_OCCLUSION_ITERATIONS = 1011;

constexpr uint32_t TIME							= 1012;

constexpr uint32_t FLOOR_HEIGHT					= 1013;
constexpr uint32_t SKY_COLOR					= 1014;

constexpr uint32_t LIGHT_DIRECTION				= 1015;
constexpr uint32_t LIGHT_COLOR					= 1016;

constexpr uint32_t CAMERA_POSITION				= 1017;
constexpr uint32_t CAMERA_VIEW					= 1018;
constexpr uint32_t CAMERA_UP					= 1019;
constexpr uint32_t CAMERA_RIGHT					= 1020;
constexpr uint32_t FOCAL_LENGTH					= 1021;

constexpr uint32_t VIGNETTE_RADIUS				= 1020;
constexpr uint32_t VIGNETTE_SMOOTHNESS			= 1021;

constexpr uint32_t SCREEN_WIDTH					= 1022;
constexpr uint32_t SCREEN_HEIGHT				= 1023;

}
