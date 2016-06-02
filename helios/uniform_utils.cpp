#include "uniform_utils.hpp"

#include <memory>
#include <unordered_map>
#include <algorithm>
#include <iostream>

#include "GL/gl3w.h"
#include "glm/gtc/type_ptr.hpp"
#include "ShaderLang.h"

/* TODO(Corralx): Find a better way to declare these, instead of hardcoding them.
   Ideally we would like to use the declared location through the layout(...) syntax,
   but glslang doesn't seems to expose that value to us
 */
/* These are the uniforms reserved for internal use, that are not exposed in the User section of the GUI */
static std::vector<std::string> default_uniforms =
{
	"_hl_epsilon",
	"_hl_z_far",
	"_hl_normal_epsilon",
	"_hl_starting_step",
	"_hl_max_iterations",
	"_hl_enable_shadow",
	"_hl_soft_shadow",
	"_hl_shadow_quality",
	"_hl_shadow_epsilon",
	"_hl_shadow_starting_step",
	"_hl_shadow_max_step",
	"_hl_enable_ambient_occlusion",
	"_hl_ambient_occlusion_step",
	"_hl_ambient_occlusion_iterations",
	"screen_width",
	"screen_height",
	"_hl_floor_height",
	"_hl_sky_color",
	"_hl_light_direction",
	"_hl_light_color",
	"_hl_camera_position",
	"_hl_camera_view",
	"_hl_camera_up",
	"_hl_camera_right",
	"_hl_focal_length",
	"time",
	"_hl_output_image"
};

/* This maps OpenGL type identiers to our enum-based uniforms types */
static const std::unordered_map<int32_t, uniform_type> uniform_type_dict =
{
	{ GL_FLOAT,				uniform_type::FLOAT  },
	{ GL_FLOAT_VEC2,		uniform_type::VEC2   },
	{ GL_FLOAT_VEC3,		uniform_type::VEC3   },
	{ GL_FLOAT_VEC4,		uniform_type::VEC4   },
	{ GL_BOOL,				uniform_type::BOOL   },
	{ GL_INT,				uniform_type::INT    },
	{ GL_INT_VEC2,			uniform_type::IVEC2  },
	{ GL_INT_VEC3,			uniform_type::IVEC3  },
	{ GL_INT_VEC4,			uniform_type::IVEC4  },
	{ GL_UNSIGNED_INT,		uniform_type::UINT   },
	{ GL_UNSIGNED_INT_VEC2, uniform_type::UVEC2  },
	{ GL_UNSIGNED_INT_VEC3, uniform_type::UVEC3  },
	{ GL_UNSIGNED_INT_VEC4, uniform_type::UVEC4  },
	{ GL_DOUBLE,			uniform_type::DOUBLE },
	{ GL_DOUBLE_VEC2,		uniform_type::DVEC2  },
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
		auto it_type = uniform_type_dict.find(gl_type);

		// Skip uniforms we can't expose because of their types
		if (it_type == uniform_type_dict.end())
		{
			std::cout << "WARNING: Ignoring uniform \"" << name <<
						 "\" because its type is not currently supported!" << std::endl;
			continue;
		}

		uniforms.emplace_back(name, it_type->second);
	}

	return std::move(uniforms);
}

void copy_uniforms_value(std::vector<uniform_t>& old_uniforms, std::vector<uniform_t>& new_uniforms)
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
				u.vec4 = it->vec4;
				break;

			case uniform_type::INT:
			case uniform_type::IVEC2:
			case uniform_type::IVEC3:
			case uniform_type::IVEC4:
			case uniform_type::UINT:
			case uniform_type::UVEC2:
			case uniform_type::UVEC3:
			case uniform_type::UVEC4:
				u.ivec4 = it->ivec4;
				break;

			case uniform_type::BOOL:
				u.boolean = it->boolean;
				break;

			default:
				assert(false);
				break;
		}
	}
}

void get_uniforms_locations(std::vector<uniform_t>& uniforms, uint32_t program)
{
	for (auto& u : uniforms)
	{
		uint32_t loc = glGetUniformLocation(program, u.name.c_str());

		// NOTE(Corralx): This should not happen because glslang optimize away the unused uniforms too
		if (loc == invalid_handle)
			std::cout << "Could not retrieve location for uniform \"" << u.name
			<< "\" (Maybe it was optimized away?)" << std::endl;

		u.location = loc;
	}
}
