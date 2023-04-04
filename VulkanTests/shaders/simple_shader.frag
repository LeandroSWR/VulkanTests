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

//layout (set = 0, binding = 1) uniform MaterialUbo 
//{
//	sampler2D albedo;
//	sampler2D normal;
//	sampler2D metalic;
//	sampler2D roughness;
//} mat;

layout(set = 0, binding = 1) uniform sampler2D image;
layout(set = 0, binding = 2) uniform sampler2D specular;
layout(set = 0, binding = 3) uniform sampler2D normal;

layout (push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

void main() 
{
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 specularLight = vec3(0.0);
	vec3 specularColor = texture(specular, fragUV).rgb;

	vec3 normalMapValue = vec3(0, 0, 1);// texture(normal, fragUV).rgb;
	normalMapValue = normalize(2.0 * normalMapValue - 1.0);

	vec3 tangent = normalize(fragTangent.xyz);
	vec3 normalWorld = normalize(fragNormalWorld);

	// Calculate the bitangent
	vec3 bitangent = cross(normalWorld, tangent) * fragTangent.w;

	// Create the TBN matrix
	mat3 TBN = mat3(tangent, bitangent, normalWorld);

	vec3 surfaceNormal = normalize(TBN * normalMapValue);

	vec3 cameraPosWorld = ubo.inverseView[3].xyz;
	vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

	for (int i = 0; i < ubo.numLights; i++) {
		PointLight light = ubo.pointLights[i];
		vec3 directionToLight = light.position.xyz - fragPosWorld;
		float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared
		directionToLight = normalize(directionToLight);
		float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
		vec3 intensity = light.color.xyz * light.color.w * attenuation;
		diffuseLight += intensity * cosAngIncidence;

		// copute specular lighting :)
		vec3 halfAngle = normalize(directionToLight + viewDirection);
		float blinnTerm = dot(surfaceNormal, halfAngle);
		blinnTerm = clamp(blinnTerm, 0, 1);
		blinnTerm = pow(blinnTerm, 32.0); // higher vlaue = sharper highlight
		specularLight += specularColor * intensity * blinnTerm;
	}

	vec3 imageColor = texture(image, fragUV).rgb;

	outColor = vec4(surfaceNormal * 0.5 + 0.5, 1.0); // vec4(diffuseLight * imageColor + specularLight * imageColor, 1.0); ////
}