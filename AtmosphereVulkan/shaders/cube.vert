#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 fragColor;

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
   gl_Position =
    camera.projectionMatrix *
    camera.viewMatrix *
    pc.modelMatrix *
    vec4(inPosition, 1.0);

    fragColor = inPosition * 0.5 + 0.5;
}