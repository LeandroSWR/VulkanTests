#version 450

layout (location = 0) in vec2 fragOffset;

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outPosition;
layout (location = 2) out vec4 outNormal;

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

layout (push_constant) uniform Push 
{
	vec4 position;
	vec4 color;
	float radius;
} push;

void main()
{
	float dist = sqrt(dot(fragOffset, fragOffset));
	if (dist >= 1.0)
	{
		discard;
	}
	
	//Workaround to have position indicators
	outAlbedo = vec4(push.color.xyz, 1.0);
	outNormal = vec4(0.0, 0.0, 1.0, 0.0);
	outPosition = vec4(0.0, 0.0, 0.0, 0.0); 
}