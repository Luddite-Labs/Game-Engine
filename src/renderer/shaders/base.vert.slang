extern static const bool UV_USED = false;
extern static const bool COLOR_USED = false;
extern static const bool POSITION_USED = false;
extern static const bool NORMAL_USED = false;

struct PlaceholderBlock {
	float pad;
};
struct TransformMatrices{
	matrix<float, 4, 4> model_view;
	matrix<float, 4, 4> proj;
};

struct Input
{
    vector<float, 3> position : TEXCOORD0;
    vector<float, 3> normal   : TEXCOORD1;
    vector<float, 3> tangent  : TEXCOORD2;
    vector<float, 3> color    : TEXCOORD3;
    vector<float, 2> uv       : TEXCOORD4;
    uint             index    : SV_VertexID;
};
struct Output
{
	vector<float, 4> color    : TEXCOORD0;
	vector<float, 3> normal   : TEXCOORD1;
	vector<float, 4> position : SV_Position;
	vector<float, 2> uv       : TEXCOORD2;
};

ParameterBlock<PlaceholderBlock> placeholderParamBlock;
ParameterBlock<TransformMatrices> transformMatrices;

[shader("vertex")]
Output main(Input input)
{
	matrix<float, 4, 4> model_view_proj = mul(transformMatrices.model_view, transformMatrices.proj);
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
		output.position = mul(float4(input.position, 1.0f), model_view_proj);
	}
	if (NORMAL_USED) {
		output.normal = input.normal;
    }
    return output;
}
