#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

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
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

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
	glm::mat4 GetViewMatrix () { return glm::lookAt (Position, Position + Front, Up); }

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
	void updateCameraVectors ()
	{
		// Calculate the new Front vector
		glm::vec3 front;
		front.x = cos (glm::radians (Yaw)) * cos (glm::radians (Pitch));
		front.y = sin (glm::radians (Pitch));
		front.z = sin (glm::radians (Yaw)) * cos (glm::radians (Pitch));
		Front = glm::normalize (front);
		// Also re-calculate the Right and Up vector
		Right = glm::normalize (glm::cross (Front,
		    WorldUp)); // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = glm::normalize (glm::cross (Right, Front));
	}
};
