#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec2 uv;

layout (location = 0) out vec3 fragPosition;
layout (location = 1) out vec2 fragUV;
layout (location = 2) out vec3 fragNormal;
layout (location = 3) out vec4 fragTangent;
layout (location = 4) out mat3 TBN;

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

layout (push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

void main() {
	vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

	gl_Position = ubo.projection * ubo.view * positionWorld;

	mat3 m3_model = mat3(push.modelMatrix);

	// Set the TBN matrix in world space
	fragNormal = normalize((push.modelMatrix * vec4(normal, 0.0)).xyz);

	vec4 tangents = vec4(normalize(m3_model * tangent.xyz), tangent.w);
	vec3 N = normalize(mat3(push.normalMatrix) * normal);
	vec3 T = tangents.xyz;
	vec3 B = cross(N, T) * tangents.w;
	TBN = mat3(T, B, N);

	fragPosition = positionWorld.xyz;
	fragUV = uv;
}