#version 450
#extension GL_KHR_vulkan_glsl: enable

const vec2 quadVertices[6] = { vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0) };

layout(location = 0) out vec2 UV;

void main()
{
    gl_Position = vec4(quadVertices[gl_VertexIndex], 0.0, 1.0);
    UV = (gl_Position.xy+vec2(1,1))/2.0;
}
