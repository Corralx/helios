#version 430 core

layout(location = 0) out vec4 color_out;

layout(location = 0) uniform uint screen_width;
layout(location = 1) uniform uint screen_height;
layout(binding = 0) uniform sampler2D source_image;

void vignette(in float a, in float b, in vec2 tex_coord, inout vec3 color)
{
	float d = distance(tex_coord, vec2(0.5, 0.5));
  	color *= smoothstep(a, b, d);
}

void main()
{
	vec2 tex_coord = gl_FragCoord.xy / vec2(screen_width, screen_height);
	vec3 color = texture(source_image, tex_coord).xyz;

	vignette(0.9, 0.07, tex_coord, color);

	color_out = vec4(color, 1.0);
}
