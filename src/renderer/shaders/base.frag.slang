import common;

extern static const bool COLOR_FACTOR_USED = false;
extern static const bool COLOR_TEXTURE_USED = false;
extern static const bool NORMAL_TEXTURE_USED = false;
extern static const bool EMISSIVE_TEXTURE_USED = false;
extern static const bool OCCLUSION_TEXTURE_USED = false;
extern static const bool METALLIC_ROUGHNESS_TEXTURE_USED = false;

struct PlaceholderBlock {
	float pad;
};

struct Material {
	vector<float, 4> color_factor;
	vector<float, 3> emissive_factor;
	float normal_scale;
	float metallic_factor;
	float roughness_factor;
};

struct TexturesBuffers { // set 2: Sampled textures, followed by storage textures, followed by storage buffers
	Texture2D<vector<float,4>> color_texture;
	Texture2D<vector<float, 3>> normal_texture ;
	Texture2D<vector<float, 4>> emissive_texture;
	Texture2D<vector<float, 4>> metallic_roughness_texture;
	Texture2D<vector<float, 4>> occlusion_texture;
	SamplerState color_sampler;
	SamplerState normal_sampler;
	SamplerState emissive_sampler;
	SamplerState metallic_roughness_sampler;
	SamplerState occlusion_sampler;
};

// set 3: Uniform buffers
struct UniformBuffer {
	Material material;
	matrix<float, 4, 4> inverted_camera_matrix;
	Layers layers;
};

struct Input {
	vector<float, 4> color;
	vector<float, 3> normal;
	vector<float, 4> position : SV_Position;
	vector<float, 2> uv;
	vector<float, 3> world_position;
};

struct Output {
	vector<float, 4> color : SV_Target0;
};

// world space
static const vector<float, 3> light_position = vector<float, 3>(-10.f, 5.f, 10.f);
static const vector<float, 3> light_direction = vector<float, 3>(1.f, 1.f, 1.f);
static const vector<float, 3> light_color = vector<float, 3>(1.f, 1.f, 1.f);
static const float PI = 3.14159265;

ParameterBlock<PlaceholderBlock> placeholderParamBlock1; // unused blocks to align params to SDL expected layout
ParameterBlock<PlaceholderBlock> placeholderParamBlock2; // unused blocks to align params to SDL expected layout
ParameterBlock<TexturesBuffers> TB;
ParameterBlock<UniformBuffer> UB;

[shader("fragment")]
Output main(Input input) {
	Output output = {};

	float epsilon = 0.00001f;
	float metallic_factor;
	float roughness_factor;
	vector<float, 3> metallic_roughness;
	vector<float, 3> color;
	vector<float, 3> normal;
	vector<float, 3> position = input.world_position;
	vector<float, 3> camera_position = UB.inverted_camera_matrix[3].xyz;
	vector<float, 3> view_dir = normalize(camera_position - position);
	vector<float, 3> incident_dir = normalize(light_position - position);
	vector<float, 3> emissive_texture_sampled = vector<float, 3>(0.f, 0.f, 0.f);
	vector<float, 3> occlusion_texture_sampled = vector<float, 3>(0.f, 0.f, 0.f);
	
	float light_distance = length(light_position - position);

	if (COLOR_FACTOR_USED) {
		if (COLOR_TEXTURE_USED) {
			color = (TB.color_texture.Sample(TB.color_sampler, input.uv) * UB.material.color_factor).xyz;
		} else {
			color = UB.material.color_factor.xyz;
		}
	}
	if (NORMAL_TEXTURE_USED) { //! didnt work as expected
		normal = TB.normal_texture.Sample(TB.normal_sampler, input.uv).xyz;
		// normal = normalize(normal * 2.0 - 1.0);
		// if(UB.material.normal_scale == 0.0){
		// 	normal.xy *= UB.material.normal_scale;
		// 	normal = normalize(normal);
		// }
	} else {
		normal = normalize(input.normal);
	}

	if (METALLIC_ROUGHNESS_TEXTURE_USED){
		metallic_roughness = TB.metallic_roughness_texture.Sample(TB.metallic_roughness_sampler, input.uv).xyz;
		metallic_factor = metallic_roughness.b;
		roughness_factor = metallic_roughness.g;
	}
	else {
		metallic_roughness = vector<float, 3>(0.f, UB.material.roughness_factor, UB.material.metallic_factor);
		metallic_factor = UB.material.metallic_factor;
		roughness_factor = UB.material.roughness_factor;
	}

	if (EMISSIVE_TEXTURE_USED){
		emissive_texture_sampled = TB.emissive_texture.Sample(TB.emissive_sampler, input.uv).xyz;
	}

	if (OCCLUSION_TEXTURE_USED){
		occlusion_texture_sampled = TB.occlusion_texture.Sample(TB.occlusion_sampler, input.uv).xyz;
	}

	vector<float, 3> half = normalize(view_dir + incident_dir);
	float n_h = dot(normal, half);
	float n_v = dot(normal, view_dir);
	float n_l = dot(normal, incident_dir);
	float h_v = dot(half, view_dir);
	float h_l = dot(half, incident_dir);
	float a_n_l = abs(n_l);
	float a_n_v = abs(n_v);
	float a_h_v = abs(h_v);
	float ior = 1.5f;
	float alpha_squared = pow(roughness_factor, 4);
	
	// specular brdf
	float G_L_N = 2  * (a_n_l) * step(0.f, h_l);
	float G_L_D = a_n_l + sqrt(alpha_squared + (1 - alpha_squared)*(n_l * n_l));

	float G_R_N = 2  * (a_n_v) * step(0.f, h_v);
	float G_R_D = a_n_v + sqrt(alpha_squared + (1 - alpha_squared) * (n_v * n_v));

	float G = (G_L_N / G_L_D) * (G_R_N / G_R_D);
	float V = G / (4 * a_n_l * a_n_v);
	float D_D_I = (n_h * n_h) * (alpha_squared - 1) + 1;
	float D = (alpha_squared * step(0.f, n_h)) / (PI * D_D_I * D_D_I);

	float specular_brdf =  V * D;

	vector<float, 3> diffuse_brdf = (1 / PI) * color;

	// dielectric brdf
	vector<float, 3> conductor_fresnel_schlick = specular_brdf * (color + (1 - color) *  pow(1 - a_h_v, 5));

	float f0 = ((1- ior)/(1+ior)) * ((1- ior)/(1+ior));
	float fr = f0 + (1 - f0) * pow(1 - a_h_v, 5);
	vector<float, 3> fresnel_mix = lerp(diffuse_brdf, vector<float, 3>(specular_brdf), fr);

	vector<float, 3> metallic_brdf = conductor_fresnel_schlick;
	vector<float, 3> dielectric_brdf = fresnel_mix;
	vector<float, 3> material_brdf = lerp(dielectric_brdf, metallic_brdf, metallic_factor);
	output.color = vector<float, 4>( material_brdf * light_color * saturate(n_l), 1.f);

	switch (UB.layers){
		case Layers::NONE:
			break;
		case Layers::ALBEDO:
			output.color = vector<float, 4>(color, 1.f);
			return output;
		case Layers::NORMAL:
			output.color = vector<float, 4>(normal, 1.f);
			return output;
		case Layers::METALLIC:
			output.color = vector<float, 4>(metallic_factor);
			return output;
		case Layers::ROUGHNESS:
			output.color = vector<float, 4>(roughness_factor);
			return output;
		case Layers::EMISSIVE:
			output.color = vector<float, 4>(emissive_texture_sampled, 1.f);
			return output;
		case Layers::OCCLUSION:
			output.color = vector<float, 4>(occlusion_texture_sampled, 1.f);
			return output;
		case Layers::SPECULAR:
			output.color = vector<float, 4>(specular_brdf);
			return output;
		case Layers::DIFFUSE:
			output.color = vector<float, 4>(diffuse_brdf, 1.f);
			return output;
		case Layers::DIELECTRIC:
			output.color = vector<float, 4>(dielectric_brdf, 1.f);
			return output;
		case Layers::METALLIC_BRDF:
			output.color = vector<float, 4>(metallic_brdf, 1.f);
			return output;
	}//! check to see if actually set

	return output;
}