import common;

extern static const bool COLOR_USED = false;
extern static const bool TANGENT_USED = false;
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
	float alpha_cutoff;
	AlphaModes alpha_mode;
	uint8_t double_sided;
};

struct TexturesBuffers { // set 2: Sampled textures, followed by storage textures, followed by storage buffers
	Sampler2D<vector<float,4>> color_texture;
	Sampler2D<vector<float, 3>> normal_texture ;
	Sampler2D<vector<float, 4>> emissive_texture;
	Sampler2D<vector<float, 4>> metallic_roughness_texture;
	Sampler2D<vector<float, 4>> occlusion_texture;
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
	vector<float, 4> tangent;
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
	input.color = clamp(input.color, 0.f, 1.f);

	float epsilon = 0.00001f;
	float metallic_factor;
	float roughness_factor;
	vector<float, 3> metallic_roughness;
	vector<float, 4> color;
	vector<float, 3> tangent;
	vector<float, 3> normal;
	vector<float, 3> position = input.world_position;
	vector<float, 3> camera_position = UB.inverted_camera_matrix[3].xyz;
	vector<float, 3> view_dir = normalize(camera_position - position);
	vector<float, 3> incident_dir = normalize(vector<float, 3>(10.f, -5.f, -10.f));//normalize(light_position - position);
	vector<float, 3> emissive_texture_sampled = vector<float, 3>(0.f, 0.f, 0.f);
	vector<float, 3> occlusion_texture_sampled = vector<float, 3>(0.f, 0.f, 0.f);
	
	float light_distance = length(light_position - position);

	color = vector<float, 4>(1.f, 1.f, 1.f, 1.f);
	if (COLOR_TEXTURE_USED) {
		color *= (TB.color_texture.Sample(input.uv) * UB.material.color_factor);
	}
	if (COLOR_USED) {
		color *= input.color;
	}
	if (COLOR_FACTOR_USED) {
		color *= UB.material.color_factor;
	}

	if (NORMAL_TEXTURE_USED) { //! didnt work as expected
		normal = TB.normal_texture.Sample(input.uv).xyz;
		normal = normal * 2.0 - 1.0;
		if (UB.material.normal_scale != 0.0) {
			normal.xy *= UB.material.normal_scale;
		}
	} else {
		normal = normalize(input.normal);		
	}

	if (TANGENT_USED) {
		tangent = input.tangent.xyz;
		vector<float, 3> vertex_normal = input.normal;
		vector<float, 3> bitangent = sign(input.tangent.w) * cross(vertex_normal, tangent);
		normal = normalize(tangent * normal.x + bitangent * normal.y + vertex_normal * normal.z);
	}
	else {
		tangent = vector<float, 3>(1.f, 1.f, 1.f);
	}

	if (METALLIC_ROUGHNESS_TEXTURE_USED){
		metallic_roughness = TB.metallic_roughness_texture.Sample(input.uv).xyz;
		metallic_factor = metallic_roughness.b;
		roughness_factor = metallic_roughness.g;
	}
	else {
		metallic_roughness = vector<float, 3>(0.f, UB.material.roughness_factor, UB.material.metallic_factor);
		metallic_factor = UB.material.metallic_factor;
		roughness_factor = UB.material.roughness_factor;
	}

	if (EMISSIVE_TEXTURE_USED){
		emissive_texture_sampled = TB.emissive_texture.Sample(input.uv).xyz;
	}

	if (OCCLUSION_TEXTURE_USED){
		occlusion_texture_sampled = TB.occlusion_texture.Sample(input.uv).xyz;
	}

	if (UB.material.double_sided == 1) {
		normal = sign(dot(view_dir, normal)) * normal;
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
	float G_L_D = a_n_l + sqrt(alpha_squared + (1.f - alpha_squared)*(n_l * n_l));

	float G_R_N = 2  * (a_n_v) * step(0.f, h_v);
	float G_R_D = a_n_v + sqrt(alpha_squared + (1.f - alpha_squared) * (n_v * n_v));

	float G = (G_L_N / G_L_D) * (G_R_N / G_R_D);
	float V = G / clamp(4.f * a_n_l * a_n_v, epsilon, 1.f);
	float D_D_I = (n_h * n_h) * (alpha_squared - 1.f) + 1.f;
	float D = (alpha_squared * step(0.f, n_h)) / (PI * D_D_I * D_D_I);

	float specular_brdf = V * D;

	vector<float, 3> diffuse_brdf = (1.0f / PI) * color.xyz;

	// Fresnel-Schlick for dielectrics (F0 = 0.04)
	float f0_dielectric = 0.04f;
	float fresnel_dielectric = f0_dielectric + (1.0f - f0_dielectric) * pow(1.0f - a_h_v, 5.0f);

	// Dielectric BRDF: (1 - F) * diffuse + F * specular (energy-conserving)
	vector<float, 3> dielectric_brdf = (1.0f - fresnel_dielectric) * diffuse_brdf + vector<float, 3>(specular_brdf * fresnel_dielectric);

	// Metallic BRDF: F(baseColor) * specular (no diffuse for metals)
	float fresnel_metallic = metallic_factor + (1.0f - metallic_factor) * pow(1.0f - a_h_v, 5.0f);
	vector<float, 3> metallic_brdf = vector<float, 3>(specular_brdf * fresnel_metallic);

	// Energy-preserving blend between dielectric and metallic
	vector<float, 3> material_brdf = lerp(dielectric_brdf, metallic_brdf, metallic_factor);
	output.color = vector<float, 4>(material_brdf * light_color * saturate(n_l), 1.f);

	switch (UB.layers){
		case Layers::NONE:
			output.color = color;
			break;
		case Layers::ALBEDO:
			output.color = color;
			break;
		case Layers::VERTEX_NORMAL:
			output.color = vector<float, 4>(normalize(input.normal), 1.f);
			break;
		case Layers::SHADING_NORMAL:
			output.color = vector<float, 4>(normal, 1.f);
			break;
		case Layers::NORMAL_TEXTURE:
			output.color = vector<float, 4>(TB.normal_texture.Sample(input.uv).xyz, 1.f);
			break;
		case Layers::METALLIC:
			output.color = vector<float, 4>(vector<float, 3>(metallic_factor), 1.f);
			break;
		case Layers::ROUGHNESS:
			output.color = vector<float, 4>(vector<float, 3>(roughness_factor), 1.f);
			break;
		case Layers::EMISSIVE:
			output.color = vector<float, 4>(emissive_texture_sampled, 1.f);
			break;
		case Layers::OCCLUSION:
			output.color = vector<float, 4>(occlusion_texture_sampled, 1.f);
			break;
		case Layers::SPECULAR:
			output.color = vector<float, 4>(vector<float, 3>(specular_brdf), 1.f);
			break;
		case Layers::DIFFUSE:
			output.color = vector<float, 4>(diffuse_brdf, 1.f);
			break;
		case Layers::DIELECTRIC:
			output.color = vector<float, 4>(dielectric_brdf, 1.f);
			break;
		case Layers::METALLIC_BRDF:
			output.color = vector<float, 4>(metallic_brdf, 1.f);
			break;
		case Layers::TANGENT:
			output.color = vector<float, 4>(tangent, 1.f);
			break;
		case Layers::UV:
			output.color = vector<float, 4>(input.uv, 0.f, 1.f);
			break;
		case Layers::DEPTH:
			output.color = vector<float, 4>(vector<float, 3>(input.position.w), 1.f);
			break;
		case Layers::VERTEX_COLOR:
			output.color = input.color;
			break;
		case Layers::ALPHA:
			output.color = vector<float, 4>(vector<float, 3>(color.w), 1.f);
			break;
	} //! check to see if actually set
	output.color = linearToSRGB(output.color);
	return output;
}