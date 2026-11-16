#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants
    {
        float angleX;
        float angleY;
        mat4 projectionMatrix; // Matches the 16 floats
        mat4 viewMatrix;
    } pc;

void main()
{
    float cy = cos(pc.angleY);
    float sy = sin(pc.angleY);

    float cx = cos(pc.angleX);
    float sx = sin(pc.angleX);

    mat3 rotationY = mat3(
         cy, 0.0, -sy,
         0.0, 1.0,  0.0,
         sy, 0.0,  cy
    );

    mat3 rotationX = mat3(
        1.0, 0.0, 0.0,
        0.0, cx, sx,
        0.0, -sx, cx
    );

    vec3 rotatedPosition = rotationX * rotationY * inPosition;

    gl_Position = pc.projectionMatrix * pc.viewMatrix * vec4(rotatedPosition, 1.0f);

    fragColor = inPosition * 0.5 + 0.5;
}