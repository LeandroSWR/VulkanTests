#version 460
#extension GL_EXT_ray_tracing : require

// Define the ray payload structure
struct RayPayload {
    vec3 color;
    float hitT;
};

layout(location = 0) rayPayloadEXT RayPayload payload;

// Binding for the top-level acceleration structure (TLAS)
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

// Access to the vertex attributes (like normals) of the intersected geometry
layout(location = 0) attribute vec3 vertexNormal;

void main() {
    // Normalize the intersection normal
    vec3 intersectionNormal = normalize(vertexNormal);
    
    // Compute the incoming ray direction
    vec3 incomingDirection = normalize(-gl_WorldRayDirectionEXT);
    
    // Compute the reflection vector
    vec3 reflectionDirection = reflect(incomingDirection, intersectionNormal);
    
    // Initialize a new payload for the reflection ray
    RayPayload reflectionPayload;
    reflectionPayload.color = vec3(0.0);
    reflectionPayload.hitT = -1.0;
    
    // Shoot a secondary ray in the reflection direction
    traceRayEXT(topLevelAS, gl_RayFlagsNoneEXT, 0xFF, 0, 1, 0, gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT, 0.01, reflectionDirection, 1000.0, 0);
    
    // Set the payload color to the reflected color
    payload.color = reflectionPayload.color;
}
