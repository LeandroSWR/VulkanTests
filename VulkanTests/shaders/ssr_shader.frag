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

layout(set = 2, binding = 0) uniform sampler2D lightingOutputTexture;
layout(set = 2, binding = 1) uniform sampler2D albedoTexture;
layout(set = 2, binding = 2) uniform sampler2D positionTexture;
layout(set = 2, binding = 3) uniform sampler2D normalTexture;
layout(set = 2, binding = 4) uniform sampler2D depthTexture;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outDebugResult;


const float PI = 3.14159265359;
const int iterationCount = 100; //Max iterations per ray
const float rayStep = 0.2f;
const float distanceBias = 0.05f; //distance at which a ray validates a hit
const float reflectionBlendingFactor = 25; //0: mirror like surfaces 5: roughness = 0.2 => no reflection 

//view space position to screen space to UV coordinates
vec2 projectedPosition(vec3 pos){
	vec4 samplePosition = ubo.projection * vec4(pos, 1.f);
	samplePosition.xy = (samplePosition.xy / samplePosition.w) * 0.5 + 0.5;
	return samplePosition.xy;
}

//view space position from UV coordinates and depth. Reconstructing the view position.
vec3 positionFromDepth(vec2 texturePos, float depth) {
	vec4 ndc = vec4((texturePos - 0.5) * 2, depth, 1.f); //converting UVs to screen space coordinates
	vec4 inversed = ubo.inverseProjection * ndc;// going back to view space
	inversed /= inversed.w; //normalization
	return inversed.xyz;
}

vec3 SSR(vec3 position, vec3 reflection)
{
	vec3 step = rayStep * reflection;
	vec3 marchingPosition = position + step; //view space position between incremented to build the ray
	float delta;
	float screenPosDepth;
	vec2 screenPosition;

	int i = 0;
	for(; i < iterationCount; i++)
	{
		screenPosition = projectedPosition(marchingPosition);
		screenPosDepth = abs(positionFromDepth(screenPosition,  texture(depthTexture, screenPosition).r).z);
		delta = abs(marchingPosition.z) - screenPosDepth;
		if (abs(delta) < distanceBias) {
			vec3 color = vec3(1);
			return texture(lightingOutputTexture, screenPosition).xyz;
		}
		else {
			//Adapts step length
			float directionSign = sign(abs(marchingPosition.z) - screenPosDepth);
			step = step * (1.0 - rayStep * max(directionSign, 0.0));
			marchingPosition += step * (-directionSign);

			// marchingPosition += step;
			// step *= 1.05;
		}
	}
	return vec3(0.0);
}


void main() {
	vec4 lightingOutput = texture(lightingOutputTexture, UV);
    vec4 albedoRoughness = texture(albedoTexture, UV);
	float depth = texture(depthTexture, UV).r;
    vec3 position =  positionFromDepth(UV, depth); //View space
    vec4 normalMetallic = texture(normalTexture, UV); //View space

    vec3 albedo = albedoRoughness.rgb;
    float roughness = albedoRoughness.a;
    vec4 normal = vec4(normalMetallic.rgb, 0.0);
    float metallic = normalMetallic.a;

    float ao = 1; //unsupported


	outColor = vec4(lightingOutput.rgb, 1.0);
	//This is one possible use of SSR
	if(roughness < 0.1)
	{
		vec3 reflectionDirection = normalize(reflect( normalize(position), normalize(normal.xyz))); // Instead of position it should have position - cameraPosition, but cameraPosition is zero in view space. :/
		vec4 reflectionColor = vec4(SSR(position, normalize(reflectionDirection)), 1.f); 
		if (reflectionColor.rgb != vec3(0.f)){
			outDebugResult = vec4(0.0, 1.0, 0.0, 1.0);
		}
		else {
			outDebugResult = vec4(1.0, 0.0, 0.0, 1.0);
		}

		outColor = mix(reflectionColor, outColor, clamp(roughness*reflectionBlendingFactor, 0, 1)); 
	}
	else {
		outDebugResult = vec4(1.0, 0.0, 0.0, 1.0);
	}
}
