#include "Input.h"

#include <algorithm>

#include <GLFW/glfw3.h>

#include "Logger.h"
#include "Window.h"


// External facing access to buttons.
namespace Input
{
std::array<InputDirector::JoystickData, JoystickCount> InputDirector::joystickData{};

template <typename Enumeration>
auto as_integer (Enumeration const value) -> typename std::underlying_type<Enumeration>::type
{
	return static_cast<typename std::underlying_type<Enumeration>::type> (value);
}

InputDirector::InputDirector (Window& window) : window (window)
{
	double xpos = 0.0, ypos = 0.0;
	glfwGetCursorPos (window.get_window_context (), &xpos, &ypos);
	mousePosition = mousePositionPrevious =
	    cml::vec2f (static_cast<float> (xpos), static_cast<float> (ypos));

	mouseControlStatus = true;
	glfwSetInputMode (window.get_window_context (), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	for (int i = 0; i < JoystickCount; i++)
	{
		joystickData[i].joystickIndex = i;
		if (glfwJoystickPresent (i))
		{
			joystickData[i].connected = true;
			joystickData[i].axes = glfwGetJoystickAxes (i, &(joystickData[i].axesCount));
			joystickData[i].buttons = glfwGetJoystickButtons (i, &(joystickData[i].buttonCount));
		}
	}
}

void InputDirector::JoystickData::Connect ()
{

	if (glfwJoystickPresent (joystickIndex))
	{
		connected = true;
		axes = glfwGetJoystickAxes (joystickIndex, &axesCount);
		buttons = glfwGetJoystickButtons (joystickIndex, &buttonCount);
	}
	else
		Disconnect ();
}

void InputDirector::JoystickData::Disconnect ()
{
	connected = false;
	axes = nullptr;
	buttons = nullptr;
}

bool InputDirector::JoystickData::IsConnected () const { return connected; }

std::array<int, JoystickCount> InputDirector::get_controller_joysticks ()
{
	std::array<int, JoystickCount> joys;

	int next = 0;
	for (int i = 0; i < JoystickCount; i++)
		if (joystickData.at (i).IsConnected ())
		{
			joys.at (next++) = joystickData.at (i).joystickIndex;
		}

	return joys;
}

float InputDirector::JoystickData::GetAxis (int index) const
{
	if (index >= 0 && index < axesCount)
		if (axes != nullptr) return axes[index];

	// Log.Error << "Tried to access axes that is out of bounds! \n";
	return 0.0f;
}

bool InputDirector::JoystickData::GetButton (int index) const
{
	if (index >= 0 && index < buttonCount)
		if (buttons != nullptr) return buttons[index];

	// Log.Error << "Tried to access button that is out of bounds! \n";
	return false;
}

bool InputDirector::get_key (KeyCode code) const { return keys[as_integer (code)]; }

bool InputDirector::get_key_down (KeyCode code) const { return keysDown[as_integer (code)]; }

bool InputDirector::get_key_up (KeyCode code) const { return keysUp[as_integer (code)]; }

bool InputDirector::get_mouse_button (int button) const { return mouseButtons[button]; }
bool InputDirector::get_mouse_button_pressed (int button) const { return mouseButtonsDown[button]; }
bool InputDirector::get_mouse_button_released (int button) const { return mouseButtonsUp[button]; }
cml::vec2f InputDirector::get_mouse_position () const { return mousePosition; }
cml::vec2f InputDirector::get_mouse_change_in_position () const { return mouseChangeInPosition; }
float InputDirector::get_mouse_scroll_x () const { return mouseScroll.x; }
float InputDirector::get_mouse_scroll_y () const { return mouseScroll.y; }
void InputDirector::set_text_input_mode () { textInputMode = true; }
void InputDirector::reset_text_input_mode () { textInputMode = false; }
bool InputDirector::get_text_input_mode () const { return textInputMode; }

bool InputDirector::get_mouse_control_status () const { return mouseControlStatus; }

void InputDirector::set_mouse_control_status (bool value)
{
	mouseControlStatus = value;
	Log.debug (fmt::format ("Mouse control status {}", value));
	if (mouseControlStatus)
	{
		glfwSetInputMode (window.get_window_context (), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else
	{
		glfwSetInputMode (window.get_window_context (), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

void InputDirector::connect_joystick (int index)
{
	if (index >= 0 && index < JoystickCount) joystickData[index].Connect ();
	// else
	//	Log.Error << "Tried to connect joystick that is out of bounds!\n";
}

void InputDirector::disconnect_joystick (int index)
{
	if (index >= 0 && index < JoystickCount) joystickData[index].Disconnect ();
	// else
	//	Log.Error << "Tried to disconnect joystick that is out of bounds!\n";
}

bool InputDirector::is_joystick_connected (int index)
{
	if (index >= 0 && index < JoystickCount) return joystickData[index].IsConnected ();

	// Log.Error << "Tried to test if an out of range joystick is connected!\n";
	return false;
}

float InputDirector::get_controller_joysticks (int id, int axis)
{
	if (id >= 0 && id < JoystickCount) return joystickData[id].GetAxis (axis);

	// Log.Error << "Tried to access joystick axis that is out of bounds!\n";
	return 0.0f;
}

bool InputDirector::get_controller_button (int id, int button)
{
	if (id >= 0 && id < JoystickCount) return joystickData[id].GetButton (button);

	// Log.Error << "Tried to access joystick button that is out of bounds!\n";
	return false;
}


void InputDirector::update_inputs ()
{
	glfwPollEvents ();

	for (int i = 0; i < JoystickCount; i++)
	{
		if (glfwJoystickPresent (i))
		{
			joystickData[i].connected = true;
			joystickData[i].axes = glfwGetJoystickAxes (i, &(joystickData[i].axesCount));
			joystickData[i].buttons = glfwGetJoystickButtons (i, &(joystickData[i].buttonCount));
		}
		else
		{
			joystickData[i].connected = false;
			joystickData[i].axes = nullptr;
			joystickData[i].buttons = nullptr;
		}
	}
}

void InputDirector::reset_released_input ()
{
	std::fill (std::begin (keysDown), std::end (keysDown), 0);
	std::fill (std::begin (keysUp), std::end (keysUp), 0);
	std::fill (std::begin (mouseButtonsUp), std::end (mouseButtonsUp), 0);
	std::fill (std::begin (mouseButtonsDown), std::end (mouseButtonsDown), 0);
	mouseScroll = cml::vec2f (0, 0);
	mouseChangeInPosition = cml::vec2f (0, 0);
}

void InputDirector::key_event (int key, int scancode, int action, int mods)
{
	switch (action)
	{
		case GLFW_PRESS:
			keys[key] = true;
			keysDown[key] = true;
			break;

		case GLFW_REPEAT: keysDown[key] = false; break;

		case GLFW_RELEASE:
			keys[key] = false;
			keysUp[key] = true;
			break;

		default: break;
	}
}

void InputDirector::mouse_button_event (int button, int action, int mods)
{
	switch (action)
	{
		case GLFW_PRESS:
			mouseButtons[button] = true;
			mouseButtonsDown[button] = true;
			break;

		case GLFW_REPEAT: mouseButtonsDown[button] = false; break;

		case GLFW_RELEASE:
			mouseButtons[button] = false;
			mouseButtonsUp[button] = true;
			break;

		default: break;
	}
}

void InputDirector::mouse_move_event (float xoffset, float yoffset)
{

	mousePosition = cml::vec2f (xoffset, yoffset);

	mouseChangeInPosition = mousePositionPrevious - mousePosition;
	mouseChangeInPosition.x *= -1; // coordinates are reversed on y axis (top left vs bottom left)

	mousePositionPrevious = mousePosition;
}

void InputDirector::mouse_scroll_event (float xoffset, float yoffset)
{
	mouseScroll = cml::vec2f (xoffset, yoffset);
}
} // namespace Input