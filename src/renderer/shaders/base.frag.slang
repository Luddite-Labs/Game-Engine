extern static const bool COLOR_FACTOR_USED   = false;
extern static const bool COLOR_TEXTURE_USED  = false;
extern static const bool NORMAL_TEXTURE_USED = false;

struct PlaceholderBlock {
	float pad;
};

struct TexturesBuffers {
	Texture2D<vector<float,4>> color_texture;
	SamplerState color_sampler;
	Texture2D<vector<float, 4>> emisssive_texture;
	SamplerState emisssive_sampler;
	Texture2D<vector<float, 3>> normal_texture ;
	SamplerState normal_sampler;
	Texture2D<vector<float, 4>> metallic_roughness_texture;
	SamplerState metallic_roughness_sampler;
	Texture2D<vector<float, 4>> occlusion_texture;
	SamplerState occlusion_sampler;
}
struct Material {
	vector<float, 4> color_factor;
	vector<float, 3> emissive_factor;
	float normal_scale;
	float metallic_factor;
	float roughness_factor;
}

struct Input {
	vector<float, 4> color;
	vector<float, 3> normal;
	vector<float, 4> position : SV_Position;
	vector<float, 2> uv;
};

struct Output {
	vector<float, 4> color : SV_Target0;
};

ParameterBlock<PlaceholderBlock> placeholderParamBlock1;
ParameterBlock<PlaceholderBlock> placeholderParamBlock2;
ParameterBlock<TexturesBuffers> texturesBuffers;
ParameterBlock<Material> material;

[shader("fragment")]
Output main(Input input) {
	Output output;
	if (COLOR_FACTOR_USED) {
		if (COLOR_TEXTURE_USED) {
			input.color = texturesBuffers.color_texture.Sample(texturesBuffers.color_sampler, input.uv) * material.color_factor;
		}
	else {
			input.color = material.color_factor;}
	}
	if (NORMAL_TEXTURE_USED) {
		input.normal = texturesBuffers.normal_texture.Sample(texturesBuffers.normal_sampler, input.uv) * material.normal_scale;
	}
	else{
		input.normal = normalize(input.normal);
	}
	output.color = input.color; 
	return output;
}
