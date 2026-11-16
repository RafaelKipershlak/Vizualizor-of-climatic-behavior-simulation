#version 450

// Input color interpolated across the triangle from the vertex shader
layout(location = 0) in vec3 fragColor;

// Final color output to your color attachment (the swapchain image)
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}