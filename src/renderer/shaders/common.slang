module common;

public enum Layers : uint32_t {
	NONE = 0,
	ALBEDO = 1 << 0,
	VERTEX_NORMAL = 1 << 1,
	METALLIC = 1 << 2,
	ROUGHNESS = 1 << 3,
	EMISSIVE = 1 << 4,
	OCCLUSION = 1 << 5,
	SPECULAR = 1 << 6,
	DIFFUSE = 1 << 7,
	DIELECTRIC = 1 << 8,
	METALLIC_BRDF = 1 << 9,
	TANGENT = 1 << 10,
	NORMAL_TEXTURE = 1 << 11,
	SHADING_NORMAL = 1 << 12,
	UV = 1 << 13,
	DEPTH = 1 << 14,
	VERTEX_COLOR = 1 << 15,
	ALPHA = 1 << 16
};

public enum AlphaModes : uint8_t {
	OPAQUE,
	MASK,
	BLEND,
};

vector<bool, 3> lessThan(vector<float, 3> color, vector<float, 3> thresh) {
	return vector<bool, 3>(color.x < thresh.x, color.y < thresh.y, color.z < thresh.z);
}

public vector<float, 4> linearToSRGB(vector<float, 4> color) {
	vector<bool, 3> cutoff = lessThan(color.xyz, vector<float, 3>(0.0031308));
	vector<float, 3> higher = vector<float, 3>(1.055f) * pow(color.xyz, vector<float, 3>(1.0f / 2.4f)) - vector<float, 3>(0.055f);
	vector<float, 3> lower = color.xyz * vector<float, 3>(12.92f);
	return vector<float, 4>(lerp(higher, lower, vector<float, 3>(cutoff)), color.w);
}

public enum LightType : uint8_t {
	DIRECTIONAL,
	POINT
}
public struct LightData {
	public float3 position_direction;
	public float3 color;
	public LightType type;
	uint8_t padding[3];
}

public static float epsilon = 0.00001f;
