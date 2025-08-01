#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

struct PointLight {
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    vec4 ambientLightColor;
    vec4 viewPos;         
    PointLight pointLights[10];
    int numLights;
} ubo;

layout(push_constant) uniform Push {
    mat4 model;
    mat4 normalMatrix;
} push;

void main() {
    vec4 wp = push.model * vec4(position,1.0);
    gl_Position = ubo.projection * ubo.view * wp;
    fragPosWorld   = wp.xyz;
    fragNormalWorld= normalize(mat3(push.normalMatrix)*normal);
    fragColor      = color;
}