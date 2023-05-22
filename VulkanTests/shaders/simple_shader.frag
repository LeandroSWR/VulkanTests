#version 450
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 fragUV;
layout (location = 4) in vec4 fragTangent;

layout (location = 0) out vec4 outColor;

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

vec3 getSurfaceNormal() {
    vec3 tangentNormal = normalize(texture(normalMap, fragUV).xyz * 2.0 - 1.0);
	vec3 tangent = normalize(fragTangent.xyz);
	vec3 normalWorld = normalize(fragNormalWorld);

	// Calculate the bitangent
	vec3 bitangent = cross(normalWorld, tangent) * fragTangent.w;

	// Create the TBN matrix
	mat3 TBN = mat3(tangent, bitangent, normalWorld);

    return normalize(TBN * tangentNormal);
}

// Schlick's approximation of Fresnel reflectance with roughness
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() 
{
	// Retrieve material properties from textures and parameters
	vec3 albedo = pow(texture(albedoMap, fragUV).rgb, vec3(2.2));
	vec3 emissive = pow(texture(emissiveMap, fragUV).rgb, vec3(2.2));
	float metallic = texture(metallicRoughnessMap, fragUV).b;
	float roughness = texture(metallicRoughnessMap, fragUV).g;
	float ao = texture(occlusionMap, fragUV).r;

	// Fake ambient light
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

	// Get the surface normal
	vec3 surfaceNormal = getSurfaceNormal();

	// Calculate the view vector
	vec3 viewVector = normalize(ubo.inverseView[3].xyz - fragPosWorld);

	// Calculate the reflection vector
	vec3 reflectionVector = reflect(-viewVector, surfaceNormal);

	// Calculate the fresnell term
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	// Initialize the outgoing light color
	vec3 Lo = vec3(0.0);

	// TODO: Calculate the lighting contribution here
	/*for (int i = 0; i < ubo.numLights; i++) {
		PointLight light = ubo.pointLights[i];
		vec3 directionToLight = light.position.xyz - fragPosWorld;
		float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared
		directionToLight = normalize(directionToLight);
		float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
		vec3 intensity = light.color.xyz * light.color.w * attenuation;
		diffuseLight += intensity * cosAngIncidence;
		
		// compute specular lighting :)
		vec3 halfAngle = normalize(directionToLight + viewDirection);
		float blinnTerm = dot(surfaceNormal, halfAngle);
		blinnTerm = clamp(blinnTerm, 0, 1);
		blinnTerm = pow(blinnTerm, 32.0); // higher vlaue = sharper highlight
		specularLight += specularColor * intensity * blinnTerm;
	}*/

	// Calculate the specular reflection component
	vec3 F = fresnelSchlickRoughness(max(dot(surfaceNormal, viewVector), 0.0), F0, roughness);
	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	// Calculate the ambient ligghting component
	vec3 diffuse = diffuseLight * albedo;
	vec3 ambient = (kD * diffuse) * ao;

	// Accumulate the final color
	vec3 color = ambient + Lo;

	// Add emissive contribution if an emissive texture is present
	if (pbrParameters.has_emissive_texture == 1) {
		color += emissive;
	}

	outColor = vec4(color, 1.0);
}

//void main() 
//{
//	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
//	vec3 specularLight = vec3(0.0);
//	vec3 specularColor = texture(metallicRoughnessTexture, fragUV).rgb;
//
//	vec3 normalMapValue = texture(normalTexture, fragUV).rgb;
//	normalMapValue = normalize(2.0 * normalMapValue - 1.0);
//
//	vec3 tangent = normalize(fragTangent.xyz);
//	vec3 normalWorld = normalize(fragNormalWorld);
//
//	// Calculate the bitangent
//	vec3 bitangent = cross(normalWorld, tangent) * fragTangent.w;
//
//	// Create the TBN matrix
//	mat3 TBN = mat3(tangent, bitangent, normalWorld);
//
//	vec3 surfaceNormal = normalize(TBN * normalMapValue);
//
//	vec3 cameraPosWorld = ubo.inverseView[3].xyz;
//	vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);
//
//	for (int i = 0; i < ubo.numLights; i++) {
//		PointLight light = ubo.pointLights[i];
//		vec3 directionToLight = light.position.xyz - fragPosWorld;
//		float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared
//		directionToLight = normalize(directionToLight);
//		float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
//		vec3 intensity = light.color.xyz * light.color.w * attenuation;
//		diffuseLight += intensity * cosAngIncidence;
//
//		// compute specular lighting :)
//		vec3 halfAngle = normalize(directionToLight + viewDirection);
//		float blinnTerm = dot(surfaceNormal, halfAngle);
//		blinnTerm = clamp(blinnTerm, 0, 1);
//		blinnTerm = pow(blinnTerm, 32.0); // higher vlaue = sharper highlight
//		specularLight += specularColor * intensity * blinnTerm;
//	}
//
//	vec3 imageColor = texture(albedoTexture, fragUV).rgb;
//
//	outColor = vec4(diffuseLight * imageColor + specularLight * imageColor, 1.0); //vec4(surfaceNormal * 0.5 + 0.5, 1.0); // 
//}