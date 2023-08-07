#version 450
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec3 fragPosition;
layout (location = 1) in vec2 fragUV;
layout (location = 2) in vec3 fragNormal;
layout (location = 3) in vec4 fragTangent;
layout (location = 4) in mat3 TBN;

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outPosition;
layout (location = 2) out vec4 outNormal;

const float PI = 3.14159265359;

struct PointLight 
{
	vec4 position; // ignore w
	vec4 color; // w is intensity
};

layout (set = 0, binding = 0) uniform GlobalUbo 
{
	mat4 projection;
	mat4 view;
	mat4 inverseView;
	mat4 inverseProjection;
	vec4 ambientLightColor;	// w is intensity
	PointLight pointLights[10];
	int numLights;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D image;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;
layout(set = 1, binding = 3) uniform sampler2D occlusionMap;
layout(set = 1, binding = 4) uniform sampler2D emissiveMap;
layout(set = 1, binding = 5) uniform PBRParameters {
	vec4 base_color_factor;
    vec3 emissive_factor;
    float metallic_factor;
    float roughness_factor;
    float scale;
    float strength;
    float alpha_cut_off;
    float alpha_mode;

    int has_base_color_texture;
    int has_metallic_roughness_texture;
    int has_normal_texture;
    int has_occlusion_texture;
    int has_emissive_texture;
} pbrParameters;

layout (push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

// Calculate the surface normal
vec3 getSurfaceNormal() {
    vec3 tangentNormal = 2.0 * normalize(texture(normalMap, fragUV).xyz) - 1.0;

	mat3 TBN = mat3(normalize(TBN[0]), normalize(TBN[1]), normalize(TBN[2]));

    return normalize(TBN * tangentNormal);
}

// Convert Srgb to Linear
vec4 SRGBtoLINEAR(vec4 srgbIn) {
	vec3 linOut = pow(srgbIn.xyz, vec3(2.2));
	return vec4(linOut, srgbIn.w);
}


void main() 
{
	// Retrieve material properties from textures and parameters
	vec4 albedo = texture(albedoMap, fragUV);
	if(albedo.a < 0.5)
	{
		discard;
	}

	float metallic = normalize(texture(metallicRoughnessMap, fragUV)).b;
	float roughness = normalize(texture(metallicRoughnessMap, fragUV)).g;

	outAlbedo = vec4(albedo.rgb, roughness);
    outPosition = vec4(mat3(ubo.view) * fragPosition, 1.0);
    outNormal = vec4(normalize(mat3(ubo.view)*fragNormal), metallic);
}
