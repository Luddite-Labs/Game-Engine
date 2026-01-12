cbuffer transform : register(b0, space1){
    float4x4 view;
    float4x4 proj;
}

struct Input
{
#ifdef POSITION_USED
       [[vk::location(0)]] float3 position : TEXCOORD0;
#endif
#ifdef NORMAL_USED
       [[vk::location(1)]] float3 normal : TEXCOORD1;
#endif
#ifdef TANGENT_USED
       [[vk::location(2)]] float3 tangent : TEXCOORD2;
#endif
#ifdef COLOR_USED
       [[vk::location(3)]] float3 color : TEXCOORD3;
#endif
#ifdef UV_USED
       [[vk::location(4)]] float2 uv : TEXCOORD4;
#endif
#ifdef INDEX_USED
    uint index : SV_VertexID;
#endif
};

struct Output
{
    float4 color : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float4 position : SV_Position;
    float2 uv : TEXCOORD2;
};


Output main(Input input)
{
    float4x4 MVP = mul(proj, view);
    Output output;
#ifdef UV_USED
    output.uv = input.uv;
#endif
#ifdef COLOR_USED
    output.color = float4(input.color, 1.0f);
#else
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
#endif
#ifdef POSITION_USED
    output.position = mul(MVP, float4(input.position, 1.0f));
#endif
#ifdef NORMAL_USED
    output.normal = input.normal;
#endif
    return output;
}
