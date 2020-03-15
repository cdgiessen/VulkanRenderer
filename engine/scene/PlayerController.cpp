#include "PlayerController.h"

#include "core/Input.h"

// Constructor with vectors
PlayerController::PlayerController (Input::InputDirector const& input, cml::vec3f position, float pitch, float yaw)
: input (input), position (position), front (cml::vec3f::forward), pitch (pitch), yaw (yaw)
{
	updateCameraVectors ();
}

void PlayerController::Update (double deltaTime)
{

	if (input.IsJoystickConnected (0))
	{
		ProcessJoystickMove (input.GetControllerAxis (0, 1),
		    input.GetControllerAxis (0, 0),
		    (input.GetControllerAxis (0, 4) + 1) / 2.0,
		    (input.GetControllerAxis (0, 5) + 1) / 2.0,
		    deltaTime);
		ProcessJoystickLook (input.GetControllerAxis (0, 3), input.GetControllerAxis (0, 4), deltaTime);

		if (input.GetControllerButton (0, 2)) ChangeCameraSpeed (MovementAxis::UP, deltaTime);
		if (input.GetControllerButton (0, 5)) ChangeCameraSpeed (MovementAxis::DOWN, deltaTime);
	}

	if (input.GetKey (Input::KeyCode::W)) ProcessKeyboard (MovementAxis::FORWARD, deltaTime);
	if (input.GetKey (Input::KeyCode::S)) ProcessKeyboard (MovementAxis::BACKWARD, deltaTime);
	if (input.GetKey (Input::KeyCode::A)) ProcessKeyboard (MovementAxis::LEFT, deltaTime);
	if (input.GetKey (Input::KeyCode::D)) ProcessKeyboard (MovementAxis::RIGHT, deltaTime);
	if (input.GetKey (Input::KeyCode::SPACE)) ProcessKeyboard (MovementAxis::UP, deltaTime);
	if (input.GetKey (Input::KeyCode::LEFT_SHIFT)) ProcessKeyboard (MovementAxis::DOWN, deltaTime);

	if (input.GetKey (Input::KeyCode::E)) ChangeCameraSpeed (MovementAxis::UP, deltaTime);
	if (input.GetKey (Input::KeyCode::Q)) ChangeCameraSpeed (MovementAxis::DOWN, deltaTime);

	if (input.GetMouseControlStatus ())
	{
		ProcessMouseMovement (input.GetMouseChangeInPosition ().x, input.GetMouseChangeInPosition ().y);
		ProcessMouseScroll (input.GetMouseScrollY (), deltaTime);
	}
}

cml::vec3f PlayerController::Position () { return position; }
cml::quatf ToQuaternion (float yaw, float pitch, float roll) // yaw (Z), pitch (Y), roll (X)
{
	// Abbreviations for the various angular functions
	float cy = cos (yaw * 0.5);
	float sy = sin (yaw * 0.5);
	float cp = cos (pitch * 0.5);
	float sp = sin (pitch * 0.5);
	float cr = cos (roll * 0.5);
	float sr = sin (roll * 0.5);

	float w = cy * cp * cr + sy * sp * sr;
	float x = cy * cp * sr - sy * sp * cr;
	float y = sy * cp * sr + cy * sp * cr;
	float z = sy * cp * cr - cy * sp * sr;
	return { w, x, y, z };
}
cml::quatf PlayerController::Rotation () { return ToQuaternion (yaw, pitch, 0); }

// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
void PlayerController::ProcessKeyboard (MovementAxis direction, float deltaTime)
{
	float velocity = MovementSpeed * deltaTime;
	switch (direction)
	{
		case MovementAxis::FORWARD: position += front * velocity; break;
		case MovementAxis::BACKWARD: position -= front * velocity; break;
		case MovementAxis::RIGHT: position += right * velocity; break;
		case MovementAxis::LEFT: position -= right * velocity; break;
		case MovementAxis::UP: position += up * velocity; break;
		case MovementAxis::DOWN: position -= up * velocity; break;
		default: break;
	}
}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void PlayerController::ProcessMouseMovement (float xoffset, float yoffset, bool constrainPitch)
{
	yaw += xoffset * MouseSensitivity;
	pitch += yoffset * MouseSensitivity;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;
	}

	// Update front, right and Up Vectors using the updated Euler angles
	updateCameraVectors ();
}

void PlayerController::ProcessJoystickMove (float x, float y, float zL, float zR, float deltaTime)
{
	float velocity = MovementSpeed * deltaTime * joystickMoveAccel;
	position += front * x * velocity;
	position += right * y * velocity;
	position -= up * zL * velocity;
	position += up * zR * velocity;
}

void PlayerController::ProcessJoystickLook (float x, float y, float deltaTime)
{
	yaw += x * joystickLookAccel * deltaTime;
	pitch += y * joystickLookAccel * deltaTime;

	if (constrainPitch)
	{
		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;
	}

	// Update front, right and Up Vectors using the updated Euler angles
	updateCameraVectors ();
}

// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void PlayerController::ProcessMouseScroll (float yoffset, float deltaTime)
{
	// if (Zoom >= 1.0f && Zoom <= 45.0f)
	//	Zoom -= yoffset;
	// if (Zoom <= 1.0f)
	//	Zoom = 1.0f;
	// if (Zoom >= 45.0f)
	//	Zoom = 45.0f;

	MovementSpeed += 5 * yoffset * MovementSpeed * deltaTime;

	if (MovementSpeed <= 0.2f) MovementSpeed = 0.2f;
}

void PlayerController::ChangeCameraSpeed (MovementAxis direction, float deltaTime)
{
	if (direction == MovementAxis::UP) MovementSpeed += 3 * MovementSpeed * deltaTime;
	if (direction == MovementAxis::DOWN) MovementSpeed -= 3 * MovementSpeed * deltaTime;
	if (MovementSpeed <= 0.2)
	{
		MovementSpeed = 0.2f;
	}
}

void PlayerController::updateCameraVectors ()
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
