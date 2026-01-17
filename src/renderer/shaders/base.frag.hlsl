cbuffer material : register(b0, space3){
 	float4 color_factor;
	float3 emissive_factor;
	float normal_scale;
	float metallic_factor;
	float roughness_factor;
}

struct Input {
    float4 color : TEXCOORD0;
    float3 normal: TEXCOORD1;
    float4 position: SV_Position;
    float2 uv : TEXCOORD2;
};

struct Output{
    float4 color: SV_Target0;
};

Texture2D<float4> color_texture : register(t0, space2);
SamplerState color_sampler : register(s0, space2);
Texture2D<float4> emisssive_texture : register(t1, space2);
SamplerState emisssive_sampler : register(s1, space2);
Texture2D<float3> normal_texture : register(t2, space2);
SamplerState normal_sampler : register(s2, space2);
Texture2D<float4> metallic_roughness_texture : register(t3, space2);
SamplerState metallic_roughness_sampler : register(s3, space2);
Texture2D<float4> occlusion_texture : register(t4, space2);
SamplerState occlusion_sampler : register(s4, space2);

Output main(Input input)
{
    Output output;
#ifdef COLOR_FACTOR_USED
    #ifdef COLOR_TEXTURE_USED
        input.color = color_texture.Sample(color_sampler, input.uv) * color_factor;
    #else
        input.color = color_factor;
    #endif
#endif
#ifdef NORMAL_TEXTURE_USED
    input.normal = normal_texture.Sample(normal_sampler, input.uv) * normal_scale;
#else    
    input.normal = normalize(input.normal);
#endif
    output.color = input.color;// float4(dot(input.normal, normalize(float3(1.0f, 1.0f, 0.0f))) * input.color.xyz, 1.0f);
    return output;
}
