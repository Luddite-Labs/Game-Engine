module common;

public enum Layers: uint32_t {
    NONE = 0,
	ALBEDO = 1 << 0,
	NORMAL = 1 << 1,
	METALLIC = 1 << 2,
	ROUGHNESS = 1 << 3,
	EMISSIVE = 1 << 4,
	OCCLUSION = 1 << 5,
    SPECULAR = 1<< 6,
    DIFFUSE = 1<< 7,
    DIELECTRIC = 1<< 8,
    METALLIC_BRDF = 1<< 9,
}

