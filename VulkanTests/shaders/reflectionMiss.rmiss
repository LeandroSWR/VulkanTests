#version 460
#extension GL_EXT_ray_tracing : require

// Define the ray payload structure
struct RayPayload {
    vec3 color;
    float hitT;
};

layout(location = 0) rayPayloadEXT RayPayload payload;

void main() {
    // Set a default environment color for missed rays
    payload.color = vec3(0.5, 0.5, 0.5); // A default gray color
}
