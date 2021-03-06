#version 430 core

layout(location = 0) out vec4 color_out;

layout(location = 1022) uniform uint screen_width;
layout(location = 1023) uniform uint screen_height;
layout(location = 1020) uniform float vignette_radius;
layout(location = 1021) uniform float vignette_smoothness;

layout(binding = 0) uniform sampler2D source_image;

void vignette(in vec2 tex_coord, inout vec3 color)
{
	float dist = distance(tex_coord, vec2(0.5, 0.5));
  	color *= smoothstep(vignette_radius, vignette_smoothness, dist);
}

void main()
{
	vec2 tex_coord = gl_FragCoord.xy / vec2(screen_width, screen_height);
	vec3 color = texture(source_image, tex_coord).xyz;

	vignette(tex_coord, color);

	color_out = vec4(color, 1.0);
}
