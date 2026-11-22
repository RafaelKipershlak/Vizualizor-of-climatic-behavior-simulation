#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

class Camera
{
public:
	using Mat4 = std::array<std::array<float, 4>, 4>;

	Camera(uint32_t surfaceWidth, uint32_t surfaceHeight);

	bool getFirstMouse() const { return firstMouse; };
	double getLastMouseX() const { return lastMouseX; };
	double getLastMouseY() const { return lastMouseY; };

	void setFirstMouse(bool newFirstMouse) { firstMouse = newFirstMouse; };
	void setLastMouseX(double newMouseX) { lastMouseX = newMouseX; };
	void setLastMouseY(double newMouseY) { lastMouseY = newMouseY; };

	void updateCameraYaw(double mouseX, double mouseY);
	void updateHorizontalPosition(float units) { cameraPosRight += units; };
	void updateVerticalPosition(float units) { cameraPosForward += units; };
	void updateCameraYaw();
	void updateCameraPitch();

	Mat4 getProjectionMatrix() const
	{
		return Mat4{ {
			{ f / aspect, 0.0f,  0.0f,                                            0.0f },
			{ 0.0f,       -f,    0.0f,                                            0.0f },
			{ 0.0f,       0.0f,  farPlane / (nearPlane - farPlane),               -1.0f },
			{ 0.0f,       0.0f,  (farPlane * nearPlane) / (nearPlane - farPlane), 0.0f }
		} };
	}

	Mat4 getViewMatrix() const
	{
		return Mat4{ {
			{ c,               s * sp,  s * cp,            0.0f },
			{ 0.0f,            cp,      -sp,               0.0f },
			{ -s,              c * sp,  c * cp,            0.0f },
			{ cameraPosRight,  0.0f,    cameraPosForward,  1.0f }
		} };
	}

	void updateAspectRatio(uint32_t width, uint32_t height);

private:
	float fovDegrees;
	float fovRadians;
	float f;
	float aspect;
	float farPlane;
	float nearPlane;
	float cameraYawDegrees;
	float cameraYaw;
	float c;
	float s;
	float cameraPitchDegrees;
	float pitch;
	float cp;
	float sp;
	double lastMouseX;
	double lastMouseY;
	bool firstMouse;
	float mouseSensitivity;
	float cameraPosForward;
	float cameraPosRight;
};
