#version 450
#extension GL_KHR_vulkan_glsl: enable

layout(location = 0) in vec2 UV;

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


layout(set = 2, binding = 0) uniform sampler2D albedoTexture;
layout(set = 2, binding = 1) uniform sampler2D positionTexture;
layout(set = 2, binding = 2) uniform sampler2D normalTexture;
layout(set = 2, binding = 3) uniform sampler2D depthTexture;

layout(location = 0) out vec4 outColor;


const float PI = 3.14159265359;

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


void main() {
    vec4 albedoRoughness = texture(albedoTexture, UV);
    vec3 position = texture(positionTexture, UV).rgb;
    vec4 normalMetallic = texture(normalTexture, UV);
    float depth = texture(depthTexture, UV).r;

    vec3 albedo = albedoRoughness.rgb;
    float roughness = albedoRoughness.a;
    vec3 normal = normalMetallic.rgb;
    float metallic = normalMetallic.a;

    float ao = 1; //unsupported

    // Fake ambient light
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

	// Calculate the view vector
	vec3 viewVector = normalize(-position);

	// Calculate the reflection vector
	vec3 reflectionVector = reflect(-viewVector, normal);

	// Calculate the fresnell term
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	// Initialize the outgoing light color
	vec3 Lo = vec3(0.0);

	// TODO: Calculate the lighting contribution here
	for (int i = 0; i < ubo.numLights; i++) {
		// Reference to the current point light
		PointLight light = ubo.pointLights[i];	
		vec3 lightPosition = mat3(ubo.view) * light.position.xyz;


		vec3 lightDirection = normalize(lightPosition - position);	// Calculate the direction from the surface point to the light source
		vec3 H = normalize(viewVector + lightDirection);					// Calculate the halfway vector between the view vector and the light vector
		float distance = length(lightPosition - position);			// Calculate the distance between the light source and the surface point
		float attenuation = 1.0 / (distance * distance);					// Calculate the attenuation factor based on the distance
		vec3 intensity = light.color.xyz * light.color.w * attenuation;		// Calculate the intensity of the light
		vec3 radiance = light.color.rgb * attenuation * intensity;			// Calculate the radiance of the light source

		float NDF = DistributionGGX(normal, H, roughness);						// Calculate the Normal Distribution Function (NDF) term
		float G = GeometrySmith(normal, viewVector, lightDirection, roughness);	// Calculate the Geometry Function (G) term
		vec3 F = fresnelSchlick(max(dot(H, viewVector), 0.0), F0);						// Calculate the Fresnel term (F)

		vec3 numerator = NDF * G * F;																								// Calculate the numerator for the specular reflection
		float denominator = 4.0 * max(dot(normal, viewVector), 0.0) * max(dot(normal, lightDirection), 0.0) + 0.0001;	// Calculate the denominator for the specular reflection
		vec3 specular = numerator / denominator;																					// Calculate the specular reflection contribution

		// Calculate the diffuse reflection (kD) and specular reflection (kS) components
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;

		// Calculate the dot product between the surface normal and the light vector
		float NdotL = max(dot(normal, lightDirection), 0.0);

		// Calculate the final output color by combining the diffuse and specular reflections
		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}

	// Calculate the specular reflection component
	vec3 F = fresnelSchlickRoughness(max(dot(normal, viewVector), 0.0), F0, roughness);
	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	// Calculate the ambient ligghting component
	vec3 diffuse = diffuseLight * albedo;
	vec3 ambient = (kD * diffuse) * ao;

	// Accumulate the final color
	vec3 color = ambient + Lo;

	// Add emissive contribution if an emissive texture is present
	/*if (pbrParameters.has_emissive_texture == 1) {
		color += emissive;
	}*/

	outColor = vec4(color, 1.0);

}
