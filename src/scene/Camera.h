#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class Camera_Movement {
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
	glm::dvec3 Position;
	glm::dvec3 Front;
	glm::dvec3 Up;
	glm::dvec3 Right;
	glm::dvec3 WorldUp;

	// Eular Angles
	double Yaw = -90.0f;
	double Pitch = 0.0f;
	
	// Camera options
	double MovementSpeed = 20.0f;
	double MouseSensitivity = 0.3f;
	double Zoom = 45.0f;

	// Joystick
	double joystickMoveAccel = 1.0f;
	double joystickLookAccel = 60.0f;

	// Constructor with vectors
	Camera(glm::dvec3 position, glm::dvec3 up, double pitch, double yaw);

	// Returns the view matrix calculated using Eular Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(Position, Position + Front, Up);
	}

	// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard(Camera_Movement direction, double deltaTime);

	void ProcessJoystickMove(double x, double y, double zL, double zR, double deltaTime);
	void ProcessJoystickLook(double x, double y, double deltaTime);


	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(double xoffset, double yoffset, bool constrainPitch = true);

	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(double yoffset, double deltaTime);

	//alter the speed of movement
	void ChangeCameraSpeed(Camera_Movement direction, double deltaTime);

private:
	// Calculates the front vector from the Camera's (updated) Eular Angles
	void updateCameraVectors()
	{
		// Calculate the new Front vector
		glm::dvec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);
		// Also re-calculate the Right and Up vector
		Right = glm::normalize(glm::cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = glm::normalize(glm::cross(Right, Front));
	}
};


