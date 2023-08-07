#version 450
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec3 fragPosition;
layout (location = 1) in vec2 fragUV;
layout (location = 2) in vec3 fragNormal;
layout (location = 3) in vec4 fragTangent;
layout (location = 4) in mat3 TBN;

layout (location = 0) out vec4 outColor;

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

// Distribution function for GGX specular term
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Geometry function for GGX specular term
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// Smith's method for combining the geometry term
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Schlick's approximation of Fresnel reflectance
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Schlick's approximation of Fresnel reflectance with roughness
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() 
{
	// Retrieve material properties from textures and parameters

	vec4 albedo = texture(albedoMap, fragUV);
	vec3 emissive = texture(emissiveMap, fragUV).rgb;
	float metallic = normalize(texture(metallicRoughnessMap, fragUV)).b;
	float roughness = normalize(texture(metallicRoughnessMap, fragUV)).g;
	float ao = texture(occlusionMap, fragUV).r;

	// Fake ambient light
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

	// Get the surface normal
	vec3 surfaceNormal = getSurfaceNormal();

	// Calculate the view vector
	vec3 viewVector = normalize(ubo.inverseView[3].xyz - fragPosition);

	// Calculate the reflection vector
	vec3 reflectionVector = reflect(-viewVector, surfaceNormal);

	// Calculate the fresnell term
	vec3 F0 = vec3(0.04);
	vec3 diffuseColor = albedo.rgb;
	F0 = mix(F0, diffuseColor, metallic);

	// Initialize the outgoing light color
	vec3 Lo = vec3(0.0);

	// TODO: Calculate the lighting contribution here
	for (int i = 0; i < ubo.numLights; i++) {
		PointLight light = ubo.pointLights[i];								// Reference to the current point light
		vec3 lightDirection = normalize(light.position.xyz - fragPosition);	// Calculate the direction from the surface point to the light source
		vec3 H = normalize(viewVector + lightDirection);					// Calculate the halfway vector between the view vector and the light vector
		float distance = length(light.position.xyz - fragPosition);			// Calculate the distance between the light source and the surface point
		float attenuation = 1.0 / (distance * distance);					// Calculate the attenuation factor based on the distance
		vec3 intensity = light.color.xyz * light.color.w * attenuation;		// Calculate the intensity of the light
		vec3 radiance = light.color.rgb * attenuation * intensity;			// Calculate the radiance of the light source

		float NDF = DistributionGGX(surfaceNormal, H, roughness);						// Calculate the Normal Distribution Function (NDF) term
		float G = GeometrySmith(surfaceNormal, viewVector, lightDirection, roughness);	// Calculate the Geometry Function (G) term
		vec3 F = fresnelSchlick(max(dot(H, viewVector), 0.0), F0);						// Calculate the Fresnel term (F)

		vec3 numerator = NDF * G * F;																								// Calculate the numerator for the specular reflection
		float denominator = 4.0 * max(dot(surfaceNormal, viewVector), 0.0) * max(dot(surfaceNormal, lightDirection), 0.0) + 0.0001;	// Calculate the denominator for the specular reflection
		vec3 specular = numerator / denominator;																					// Calculate the specular reflection contribution

		// Calculate the diffuse reflection (kD) and specular reflection (kS) components
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;

		// Calculate the dot product between the surface normal and the light vector
		float NdotL = max(dot(surfaceNormal, lightDirection), 0.0);

		// Calculate the final output color by combining the diffuse and specular reflections
		Lo += (kD * diffuseColor / PI + specular) * radiance * NdotL;
	}

	// Calculate the specular reflection component
	vec3 F = fresnelSchlickRoughness(max(dot(surfaceNormal, viewVector), 0.0), F0, roughness);
	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	// Calculate the ambient ligghting component
	vec3 diffuse = diffuseLight * diffuseColor;
	vec3 ambient = (kD * diffuse) * ao;

	// Accumulate the final color
	vec3 color = ambient + Lo;

	// Add emissive contribution if an emissive texture is present
	if (pbrParameters.has_emissive_texture == 1) {
		color += emissive;
	}

	outColor = vec4(color, albedo.a);
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