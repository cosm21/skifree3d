#version 330

struct light_source {
	vec3 position;
	vec3 color;

	bool spotlight;
	vec3 direction;
	float cut_off;

	float const_att;
	float lin_att;
	float sq_att;
};

in vec3 world_position;
in vec3 world_normal;

in vec2 texcoord;
in vec3 color;
in vec3 normal;

uniform sampler2D texture;

uniform float global_light_att;
uniform	float material_kd;
uniform	float material_ks;
uniform	int material_shininess;

uniform	vec3 eye_position;
uniform light_source lights[255];
uniform int num_lights;

layout(location = 0) out vec4 out_color;

float light_contribution(int index)
{
	vec3 light_position = lights[index].position;
	vec3 light_direction = lights[index].direction;
	bool spotlight = lights[index].spotlight;
	float cut_off = lights[index].cut_off;

	float const_att = lights[index].const_att;
	float lin_att = lights[index].lin_att;
	float sq_att = lights[index].sq_att;

	float diffuse_light = 0;
	float specular_light = 0;

	vec3 L = normalize(light_position - world_position);
	vec3 V = normalize(eye_position - world_position);
	vec3 H = normalize(L + V);
	vec3 N = world_normal;

	diffuse_light = material_kd * max(dot(N, L), 0);

	vec3 R = reflect(-L, N);
	int n = material_shininess;

	float receive_light = dot(N, L) > 0 ? 1 : 0;

	if (diffuse_light > 0)
	{
		specular_light = material_ks * receive_light * pow(max(dot(N, H), 0), n);
	}

	float light_att_factor = 1.f;
	if (spotlight) {
		float spot_light = dot(-L, light_direction);
		float spot_light_limit = cos(cut_off);
		float linear_att = (spot_light - spot_light_limit) / (1.0f - spot_light_limit);

		if (spot_light > cos(cut_off)) {
			light_att_factor = pow(linear_att, 2);
		} else {
			light_att_factor = 0.f;
		}
	}

	float dist = distance(light_position, world_position);
	float attenuation_factor = 1 / (const_att + lin_att * dist + sq_att * dist * dist);
	float light = 2 * light_att_factor * attenuation_factor * (diffuse_light + specular_light);

	return light;
}

float global_light_contribution()
{
	float diffuse_light = 0;
	float specular_light = 0;

	vec3 L = vec3(0, 1, 0);
	vec3 N = world_normal;

	diffuse_light = material_kd * max(dot(N, L), 0);

	// Componenta speculara a fost omisa pentru ca genera
	// o pata luminoasa sub camera
	float light = diffuse_light;

	return global_light_att * light;
}

void main()
{
	float al = 0.25 * material_kd;
	vec3 light = vec3(al, al, al);
	for (int i = 0; i < num_lights; i++) {
		light += lights[i].color * light_contribution(i);
	}
	light += vec3(1.f) * global_light_contribution();

	vec4 lcolor = texture2D(texture, texcoord);
	if (lcolor.a <= 0.3f) {
		discard;
	}
	out_color = vec4(light, 1.f) * lcolor;
}
