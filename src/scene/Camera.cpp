#include "Camera.h"

// Constructor with vectors
Camera::Camera(glm::vec3 position, glm::vec3 up, float pitch, float yaw) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), Pitch(pitch), Yaw(yaw)
{
	Position = position;
	WorldUp = up;
	updateCameraVectors();
}

// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
	float velocity = MovementSpeed * deltaTime;
	if (direction == Camera_Movement::FORWARD)
		Position += Front * velocity;
	if (direction == Camera_Movement::BACKWARD)
		Position -= Front * velocity;
	if (direction == Camera_Movement::LEFT)
		Position -= Right * velocity;
	if (direction == Camera_Movement::RIGHT)
		Position += Right * velocity;
	if (direction == Camera_Movement::UP)
		Position += Up * velocity;
	if (direction == Camera_Movement::DOWN)
		Position -= Up* velocity;
}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
	Yaw += xoffset * MouseSensitivity;
	Pitch += yoffset * MouseSensitivity;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (Pitch > 89.0f)
			Pitch = 89.0f;
		if (Pitch < -89.0f)
			Pitch = -89.0f;
	}

	// Update Front, Right and Up Vectors using the updated Eular angles
	updateCameraVectors();
}

void Camera::ProcessJoystickMove(float x, float y, float zL, float zR, float deltaTime) {
	float velocity = MovementSpeed * deltaTime * joystickMoveAccel;
	Position += Front * x * velocity;
	Position += Right * y * velocity;
	Position -= Up * zL * velocity;
	Position += Up * zR * velocity;
}

void Camera::ProcessJoystickLook(float x, float y, float deltaTime) {
	Yaw += x * joystickLookAccel*deltaTime;
	Pitch += y * joystickLookAccel*deltaTime;

	if (true /*constrainPitch*/)
	{
		if (Pitch > 89.0f)
			Pitch = 89.0f;
		if (Pitch < -89.0f)
			Pitch = -89.0f;
	}

	// Update Front, Right and Up Vectors using the updated Eular angles
	updateCameraVectors();
}

// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset, float deltaTime)
{
	//if (Zoom >= 1.0f && Zoom <= 45.0f)
	//	Zoom -= yoffset;
	//if (Zoom <= 1.0f)
	//	Zoom = 1.0f;
	//if (Zoom >= 45.0f)
	//	Zoom = 45.0f;

	MovementSpeed += 5 * yoffset * MovementSpeed * deltaTime;
	
	if (MovementSpeed <= 0.2f)
		MovementSpeed = 0.2f;
	
}

void Camera::ChangeCameraSpeed(Camera_Movement direction, float deltaTime) {
	if (direction == Camera_Movement::UP)
		MovementSpeed += 3 * MovementSpeed * deltaTime;
	if (direction == Camera_Movement::DOWN)
		MovementSpeed -= 3 * MovementSpeed * deltaTime;
	if (MovementSpeed <= 0.2) {
		MovementSpeed = 0.2f;
	}
}