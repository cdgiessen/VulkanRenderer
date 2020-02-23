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
	glfwGetCursorPos (window.GetWindowContext (), &xpos, &ypos);
	mousePosition = mousePositionPrevious = cml::vec2d (xpos, ypos);

	mouseControlStatus = true;
	glfwSetInputMode (window.GetWindowContext (), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

std::array<int, JoystickCount> InputDirector::GetConnectedJoysticks ()
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

bool InputDirector::GetKey (KeyCode code) const { return keys[as_integer (code)]; }

bool InputDirector::GetKeyDown (KeyCode code) const { return keysDown[as_integer (code)]; }

bool InputDirector::GetKeyUp (KeyCode code) const { return keysUp[as_integer (code)]; }

bool InputDirector::GetMouseButton (int button) const { return mouseButtons[button]; }
bool InputDirector::GetMouseButtonPressed (int button) const { return mouseButtonsDown[button]; }
bool InputDirector::GetMouseButtonReleased (int button) const { return mouseButtonsUp[button]; }
cml::vec2d InputDirector::GetMousePosition () const { return mousePosition; }
cml::vec2d InputDirector::GetMouseChangeInPosition () const { return mouseChangeInPosition; }
double InputDirector::GetMouseScrollX () const { return mouseScroll.x; }
double InputDirector::GetMouseScrollY () const { return mouseScroll.y; }
void InputDirector::SetTextInputMode () { textInputMode = true; }
void InputDirector::ResetTextInputMode () { textInputMode = false; }
bool InputDirector::GetTextInputMode () const { return textInputMode; }

bool InputDirector::GetMouseControlStatus () const { return mouseControlStatus; }

void InputDirector::SetMouseControlStatus (bool value)
{
	mouseControlStatus = value;
	Log.Debug (fmt::format ("Mouse control status {}", value));
	if (mouseControlStatus)
	{
		glfwSetInputMode (window.GetWindowContext (), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else
	{
		glfwSetInputMode (window.GetWindowContext (), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

void InputDirector::ConnectJoystick (int index)
{
	if (index >= 0 && index < JoystickCount) joystickData[index].Connect ();
	// else
	//	Log.Error << "Tried to connect joystick that is out of bounds!\n";
}

void InputDirector::DisconnectJoystick (int index)
{
	if (index >= 0 && index < JoystickCount) joystickData[index].Disconnect ();
	// else
	//	Log.Error << "Tried to disconnect joystick that is out of bounds!\n";
}

bool InputDirector::IsJoystickConnected (int index)
{
	if (index >= 0 && index < JoystickCount) return joystickData[index].IsConnected ();

	// Log.Error << "Tried to test if an out of range joystick is connected!\n";
	return false;
}

float InputDirector::GetControllerAxis (int id, int axis)
{
	if (id >= 0 && id < JoystickCount) return joystickData[id].GetAxis (axis);

	// Log.Error << "Tried to access joystick axis that is out of bounds!\n";
	return 0.0f;
}

bool InputDirector::GetControllerButton (int id, int button)
{
	if (id >= 0 && id < JoystickCount) return joystickData[id].GetButton (button);

	// Log.Error << "Tried to access joystick button that is out of bounds!\n";
	return false;
}


void InputDirector::UpdateInputs ()
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

void InputDirector::ResetReleasedInput ()
{
	std::fill (std::begin (keysDown), std::end (keysDown), 0);
	std::fill (std::begin (keysUp), std::end (keysUp), 0);
	std::fill (std::begin (mouseButtonsUp), std::end (mouseButtonsUp), 0);
	std::fill (std::begin (mouseButtonsDown), std::end (mouseButtonsDown), 0);
	mouseScroll = cml::vec2d (0, 0);
	mouseChangeInPosition = cml::vec2d (0, 0);
}

void InputDirector::keyEvent (int key, int scancode, int action, int mods)
{
	switch (action)
	{
		case GLFW_PRESS:
			keys[key] = true;
			keysDown[key] = true;
			break;

		case GLFW_REPEAT:
			keysDown[key] = false;
			break;

		case GLFW_RELEASE:
			keys[key] = false;
			keysUp[key] = true;
			break;

		default:
			break;
	}
}

void InputDirector::mouseButtonEvent (int button, int action, int mods)
{
	switch (action)
	{
		case GLFW_PRESS:
			mouseButtons[button] = true;
			mouseButtonsDown[button] = true;
			break;

		case GLFW_REPEAT:
			mouseButtonsDown[button] = false;
			break;

		case GLFW_RELEASE:
			mouseButtons[button] = false;
			mouseButtonsUp[button] = true;
			break;

		default:
			break;
	}
}

void InputDirector::mouseMoveEvent (double xoffset, double yoffset)
{

	mousePosition = cml::vec2d (xoffset, yoffset);

	mouseChangeInPosition = mousePositionPrevious - mousePosition;
	mouseChangeInPosition.x *= -1; // coordinates are reversed on y axis (top left vs bottom left)

	mousePositionPrevious = mousePosition;
}

void InputDirector::mouseScrollEvent (double xoffset, double yoffset)
{
	mouseScroll = cml::vec2d (xoffset, yoffset);
}
} // namespace Input