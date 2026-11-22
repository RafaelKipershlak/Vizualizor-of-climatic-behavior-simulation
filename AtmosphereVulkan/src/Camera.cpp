#include "Camera.h"

Camera::Camera(uint32_t surfaceWidth, uint32_t surfaceHeight)
{
    fovDegrees = 60.0f;
    fovRadians = fovDegrees * (3.14159265358979323846f / 180.0f);
    f = 1.0f / std::tan(fovRadians / 2.0f);
    aspect = static_cast<float>(surfaceWidth) / static_cast<float>(surfaceHeight);
    farPlane = 100.0f;
    nearPlane = 0.1f;

    cameraYawDegrees = 0.0f;
    cameraYaw = cameraYawDegrees * (3.14159265358979323846f / 180.0f);
    c = std::cos(cameraYaw);
    s = std::sin(cameraYaw);

    cameraPitchDegrees = 0.0f; // 1. Add pitch tracking
    pitch = cameraPitchDegrees * (3.14159265358979323846f / 180.0f);
    cp = std::cos(pitch);
    sp = std::sin(pitch);

    lastMouseX = 0.0;
    lastMouseY = 0.0;

    firstMouse = true;

    mouseSensitivity = 0.1f;

    cameraPosForward = -5.0f;
    cameraPosRight = 0.0f;
}

void Camera::updateCameraYaw(double mouseX, double mouseY) 
{
    if (firstMouse)
    {
        lastMouseX = mouseX;
        lastMouseY = mouseY;
        firstMouse = false;
    }

    double deltaX = mouseX - lastMouseX;
    double deltaY = mouseY - lastMouseY;

    lastMouseX = mouseX;
    lastMouseY = mouseY;

    // Update camera angles
    cameraYawDegrees -= static_cast<float>(deltaX) * mouseSensitivity;
    cameraPitchDegrees += static_cast<float>(deltaY) * mouseSensitivity;
}

void Camera::updateCameraYaw()
{
    cameraYaw = cameraYawDegrees * (3.14159265358979323846f / 180.0f);
    c = std::cos(cameraYaw);
    s = std::sin(cameraYaw);
}

void Camera::updateCameraPitch()
{
    pitch = cameraPitchDegrees * (3.14159265358979323846f / 180.0f);
    cp = std::cos(pitch);
    sp = std::sin(pitch);
}

void Camera::updateAspectRatio(uint32_t width, uint32_t height)
{
    if (height == 0)
        return;

    aspect = static_cast<float>(width) / static_cast<float>(height);
}