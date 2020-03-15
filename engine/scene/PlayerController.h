#pragma once

#include "cml/cml.h"

namespace Input
{
class InputDirector;
}

class PlayerController
{
	Input::InputDirector const& input;

	// Camera Attributes
	cml::vec3f position;
	cml::vec3f front;
	cml::vec3f up;
	cml::vec3f right;
	cml::vec3f world_up = cml::vec3f::up;

	bool constrainPitch = true;

	// Euler Angles
	float pitch = 0.0f;
	float yaw = -90.0f;

	// Camera options
	float MovementSpeed = 20.0f;
	float MouseSensitivity = 0.3f;
	float Zoom = 45.0f;

	// Joystick
	float joystickMoveAccel = 1.0f;
	float joystickLookAccel = 60.0f;

	public:
	// Constructor with vectors
	PlayerController (Input::InputDirector const& input, cml::vec3f position, float pitch, float yaw);

	void Update (double deltaTime);
	cml::vec3f Position ();
	cml::quatf Rotation ();

	private:
	enum class MovementAxis
	{
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		UP,
		DOWN
	};
	// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard (MovementAxis direction, float deltaTime);

	void ProcessJoystickMove (float x, float y, float zL, float zR, float deltaTime);
	void ProcessJoystickLook (float x, float y, float deltaTime);

	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement (float xoffset, float yoffset, bool constrainPitch = true);

	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll (float yoffset, float deltaTime);

	// alter the speed of movement
	void ChangeCameraSpeed (MovementAxis direction, float deltaTime);

	// Calculates the front vector from the Camera's (updated) Euler Angles
	void updateCameraVectors ();
};
