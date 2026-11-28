
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;

layout(push_constant) uniform PushConstants
{
    mat4 modelMatrix;
} pc;

layout(set = 0, binding = 0) uniform CameraData
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
} camera;

void main()
{
    vec4 worldPosition = pc.modelMatrix * vec4(inPosition, 1.0);

    vec4 viewPosition = camera.viewMatrix * worldPosition;

    fragPosition = viewPosition.xyz;
    fragUV = inUV;
    fragNormal = normalize(
        mat3(camera.viewMatrix * pc.modelMatrix) * inNormal
    );

    gl_Position = camera.projectionMatrix * viewPosition;
}
