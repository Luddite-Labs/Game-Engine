import common;

extern static const bool UV_USED = false;
extern static const bool COLOR_USED = false;
extern static const bool POSITION_USED = false;
extern static const bool NORMAL_USED = false;

struct TransformMatrices{
	matrix<float, 4, 4> model;
	matrix<float, 4, 4> view;
	matrix<float, 4, 4> proj;
};

struct TexturesBuffers { // set 0: Sampled textures, followed by storage textures, followed by storage buffers
	float pad; // unused pad in order to consume descriptor set and align shader params to SDL expected layout
};
struct UniformBuffers { // set 1: Uniform buffers
	TransformMatrices transformMatrices;
	Layers layers; 
};

struct Input
{
    vector<float, 3> position;
    vector<float, 3> normal;
    vector<float, 3> tangent;
    vector<float, 3> color;
    vector<float, 2> uv;
    uint index : SV_VertexID;
};
struct Output
{
	vector<float, 4> color;
	vector<float, 3> normal;
	vector<float, 4> position : SV_Position;
	vector<float, 2> uv;
	vector<float, 3> world_position;
};

ParameterBlock<TexturesBuffers> TB;
ParameterBlock<UniformBuffers> UB;

[shader("vertex")]
Output main(Input input)
{
	matrix<float, 4, 4> model_view = mul(UB.transformMatrices.view, UB.transformMatrices.model);
	matrix<float, 4, 4> model_view_proj = mul(UB.transformMatrices.proj, model_view);
	Output output;
	if (UV_USED) {
		output.uv = input.uv;
	}
	if (COLOR_USED) {
		output.color = float4(input.color, 1.0f);
	}
    else {
		output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	if (POSITION_USED) {
		output.position = mul(model_view_proj, vector<float, 4>(input.position, 1.0f));
		output.world_position =  mul(UB.transformMatrices.model, vector<float, 4>(input.position, 1.0f)).xyz;
	}
	if (NORMAL_USED) {
		output.normal = mul(UB.transformMatrices.model, vector<float, 4>(input.normal, 0.0f)).xyz;
	}
    return output;
}
