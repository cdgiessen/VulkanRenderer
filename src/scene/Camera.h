#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

enum class Camera_Movement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class Camera
{
	public:
	// Camera Attributes
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up, Down;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	bool is_upside_down = false;
	bool constrainPitch = true;

	// Eular Angles
	float Yaw = -90.0f;
	float Pitch = 0.0f;

	// Camera options
	float MovementSpeed = 20.0f;
	float MouseSensitivity = 0.3f;
	float Zoom = 45.0f;

	// Joystick
	float joystickMoveAccel = 1.0f;
	float joystickLookAccel = 60.0f;

	// Constructor with vectors
	Camera (glm::vec3 position, glm::vec3 up, float pitch, float yaw);

	// Returns the view matrix calculated using Eular Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix ();

	// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard (Camera_Movement direction, float deltaTime);

	void ProcessJoystickMove (float x, float y, float zL, float zR, float deltaTime);
	void ProcessJoystickLook (float x, float y, float deltaTime);


	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement (float xoffset, float yoffset, bool constrainPitch = true);

	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll (float yoffset, float deltaTime);

	// alter the speed of movement
	void ChangeCameraSpeed (Camera_Movement direction, float deltaTime);

	private:
	// Calculates the front vector from the Camera's (updated) Eular Angles
	void updateCameraVectors ();
};
