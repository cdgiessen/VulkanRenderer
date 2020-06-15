#include "PlayerController.h"

#include "core/Input.h"

// Constructor with vectors
PlayerController::PlayerController (Input::InputDirector const& input, cml::vec3f position, float pitch, float yaw)
: input (input), m_position (position), front (cml::vec3f::forward), pitch (pitch), yaw (yaw)
{
	update_camera_vectors ();
}

void PlayerController::update (float deltaTime)
{

	if (input.is_joystick_connected (0))
	{
		process_joystick_move (input.get_controller_joysticks (0, 1),
		    input.get_controller_joysticks (0, 0),
		    (input.get_controller_joysticks (0, 4) + 1) / 2.0f,
		    (input.get_controller_joysticks (0, 5) + 1) / 2.0f,
		    deltaTime);
		process_joystick_look (
		    input.get_controller_joysticks (0, 3), input.get_controller_joysticks (0, 4), deltaTime);

		if (input.get_controller_button (0, 2)) change_camera_speed (MovementAxis::UP, deltaTime);
		if (input.get_controller_button (0, 5)) change_camera_speed (MovementAxis::DOWN, deltaTime);
	}

	if (input.get_key (Input::KeyCode::W)) process_keyboard (MovementAxis::FORWARD, deltaTime);
	if (input.get_key (Input::KeyCode::S)) process_keyboard (MovementAxis::BACKWARD, deltaTime);
	if (input.get_key (Input::KeyCode::A)) process_keyboard (MovementAxis::LEFT, deltaTime);
	if (input.get_key (Input::KeyCode::D)) process_keyboard (MovementAxis::RIGHT, deltaTime);
	if (input.get_key (Input::KeyCode::SPACE)) process_keyboard (MovementAxis::UP, deltaTime);
	if (input.get_key (Input::KeyCode::LEFT_SHIFT))
		process_keyboard (MovementAxis::DOWN, deltaTime);

	if (input.get_key (Input::KeyCode::E)) change_camera_speed (MovementAxis::UP, deltaTime);
	if (input.get_key (Input::KeyCode::Q)) change_camera_speed (MovementAxis::DOWN, deltaTime);

	if (input.get_mouse_control_status ())
	{
		process_mouse_movement (input.get_mouse_change_in_position ());
		process_mouse_scroll (input.get_mouse_scroll_y (), deltaTime);
	}
}

cml::vec3f PlayerController::position () { return m_position; }
cml::quatf to_quaternion (float yaw, float pitch, float roll) // yaw (Z), pitch (Y), roll (X)
{
	// Abbreviations for the various angular functions
	float cy = cos (yaw * 0.5f);
	float sy = sin (yaw * 0.5f);
	float cp = cos (pitch * 0.5f);
	float sp = sin (pitch * 0.5f);
	float cr = cos (roll * 0.5f);
	float sr = sin (roll * 0.5f);

	float w = cy * cp * cr + sy * sp * sr;
	float x = cy * cp * sr - sy * sp * cr;
	float y = sy * cp * sr + cy * sp * cr;
	float z = sy * cp * cr - cy * sp * sr;
	return { w, x, y, z };
}
cml::quatf PlayerController::rotation () { return to_quaternion (yaw, pitch, 0); }

// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
void PlayerController::process_keyboard (MovementAxis direction, float deltaTime)
{
	float velocity = movement_speed * deltaTime;
	switch (direction)
	{
		case MovementAxis::FORWARD: m_position += front * velocity; break;
		case MovementAxis::BACKWARD: m_position -= front * velocity; break;
		case MovementAxis::RIGHT: m_position += right * velocity; break;
		case MovementAxis::LEFT: m_position -= right * velocity; break;
		case MovementAxis::UP: m_position += up * velocity; break;
		case MovementAxis::DOWN: m_position -= up * velocity; break;
		default: break;
	}
}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void PlayerController::process_mouse_movement (cml::vec2f offset, bool constrainPitch)
{
	yaw += offset.x * mouse_sensitivity;
	pitch += offset.y * mouse_sensitivity;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;
	}

	// Update front, right and Up Vectors using the updated Euler angles
	update_camera_vectors ();
}

void PlayerController::process_joystick_move (float x, float y, float zL, float zR, float deltaTime)
{
	float velocity = movement_speed * deltaTime * joystick_move_accel;
	m_position += front * x * velocity;
	m_position += right * y * velocity;
	m_position -= up * zL * velocity;
	m_position += up * zR * velocity;
}

void PlayerController::process_joystick_look (float x, float y, float deltaTime)
{
	yaw += x * joystick_look_accel * deltaTime;
	pitch += y * joystick_look_accel * deltaTime;

	if (constrainPitch)
	{
		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;
	}

	// Update front, right and Up Vectors using the updated Euler angles
	update_camera_vectors ();
}

// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void PlayerController::process_mouse_scroll (float yoffset, float deltaTime)
{
	// if (zoom >= 1.0f && zoom <= 45.0f)
	//	zoom -= yoffset;
	// if (zoom <= 1.0f)
	//	zoom = 1.0f;
	// if (zoom >= 45.0f)
	//	zoom = 45.0f;

	movement_speed += 5 * yoffset * movement_speed * deltaTime;

	if (movement_speed <= 0.2f) movement_speed = 0.2f;
}

void PlayerController::change_camera_speed (MovementAxis direction, float deltaTime)
{
	if (direction == MovementAxis::UP) movement_speed += 3 * movement_speed * deltaTime;
	if (direction == MovementAxis::DOWN) movement_speed -= 3 * movement_speed * deltaTime;
	if (movement_speed <= 0.2)
	{
		movement_speed = 0.2f;
	}
}

void PlayerController::update_camera_vectors ()
{
	// Calculate the new front vector
	cml::vec3f front;
	front.x = cos (cml::radians (yaw)) * cos (cml::radians (pitch));
	front.y = sin (cml::radians (pitch));
	front.z = sin (cml::radians (yaw)) * cos (cml::radians (pitch));
	front = cml::normalize (front);
	// Also re-calculate the right and up vector
	// Normalize the vectors, because their length gets closer to 0 the more you look up or down
	// which results in slower movement.
	right = cml::normalize (cml::cross (front, world_up));
	up = cml::normalize (cml::cross (right, front));
}
