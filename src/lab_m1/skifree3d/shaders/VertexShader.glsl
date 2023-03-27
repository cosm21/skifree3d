#version 330

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texture_coord;
layout(location = 3) in vec3 v_color;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

uniform bool keep_still;
uniform vec2 texture_offset;

out vec3 world_position;
out vec3 world_normal;

out vec2 texcoord;
out vec3 color;
out vec3 normal;

void main()
{
	world_position = (Model * vec4(v_position, 1)).xyz;
	world_normal = normalize(mat3(Model) * v_normal);

	texcoord = v_texture_coord;
	color = v_color;
	normal = v_normal;

	if (keep_still)
		texcoord += texture_offset;

	gl_Position = Projection * View * Model * vec4(v_position, 1.0);
}
