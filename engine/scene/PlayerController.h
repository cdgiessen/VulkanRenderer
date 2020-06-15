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
	cml::vec3f m_position;
	cml::vec3f front;
	cml::vec3f up;
	cml::vec3f right;
	cml::vec3f world_up = cml::vec3f::up;

	bool constrainPitch = true;

	// Euler Angles
	float pitch = 0.0f;
	float yaw = -90.0f;

	// Camera options
	float movement_speed = 20.0f;
	float mouse_sensitivity = 0.3f;
	float zoom = 45.0f;

	// Joystick
	float joystick_move_accel = 1.0f;
	float joystick_look_accel = 60.0f;

	public:
	// Constructor with vectors
	PlayerController (Input::InputDirector const& input, cml::vec3f position, float pitch, float yaw);

	void update (float deltaTime);
	cml::vec3f position ();
	cml::quatf rotation ();

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
	void process_keyboard (MovementAxis direction, float deltaTime);

	void process_joystick_move (float x, float y, float zL, float zR, float deltaTime);
	void process_joystick_look (float x, float y, float deltaTime);

	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void process_mouse_movement (cml::vec2f offset, bool constrainPitch = true);

	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void process_mouse_scroll (float yoffset, float deltaTime);

	// alter the speed of movement
	void change_camera_speed (MovementAxis direction, float deltaTime);

	// Calculates the front vector from the Camera's (updated) Euler Angles
	void update_camera_vectors ();
};
