#include "Input.h"

#include <algorithm>
#include <GLFW/glfw3.h>


//External facing access to buttons. 
namespace Input {
	template <typename Enumeration>
	auto as_integer(Enumeration const value) -> typename std::underlying_type<Enumeration>::type
	{
		return static_cast<typename std::underlying_type<Enumeration>::type>(value);
	}

	InputDirector inputDirector;

	bool GetKey(KeyCode code) {
		return inputDirector.GetKey(code);
	}

	bool GetKeyDown(KeyCode code) {
		return inputDirector.GetKeyDown(code);
	}

	bool GetKeyUp(KeyCode code) {
		return inputDirector.GetKeyUp(code);
	}

	bool GetMouseButton(int button) {
		return inputDirector.GetMouseButton(button);
	}
	bool GetMouseButtonPressed(int button) {
		return inputDirector.GetMouseButtonPressed(button);
	}
	bool GetMouseButtonReleased(int button) {
		return inputDirector.GetMouseButtonReleased(button);
	}
	glm::vec2 GetMousePosition() {
		return inputDirector.GetMousePosition();
	}
	glm::vec2 GetMouseChangeInPosition() {
		return inputDirector.GetMouseChangeInPosition();
	}
	float GetMouseScrollX() {
		return inputDirector.GetMouseScrollX();
	}
	float GetMouseScrollY() {
		return inputDirector.GetMouseScrollY();
	}

	InputDirector::InputDirector() :
		mousePosition(glm::vec2(0, 0)), mousePositionPrevious(glm::vec2(0, 0)), 
		mouseChangeInPosition(glm::vec2(0, 0)), mouseScroll(glm::vec2(0, 0)) 
	{
	}

	bool InputDirector::GetKey(KeyCode code) {
		return keys[as_integer(code)];
	}

	bool InputDirector::GetKeyDown(KeyCode code) {
		return keysDown[as_integer(code)];
	}

	bool InputDirector::GetKeyUp(KeyCode code) {
		return keysUp[as_integer(code)];
	}

	bool InputDirector::GetMouseButton(int button) {
		return mouseButtons[button];
	}
	bool InputDirector::GetMouseButtonPressed(int button) {
		return mouseButtonsDown[button];
	}
	bool InputDirector::GetMouseButtonReleased(int button) {
		return mouseButtonsUp[button];
	}
	glm::vec2 InputDirector::GetMousePosition() {
		return mousePosition;
	}
	glm::vec2 InputDirector::GetMouseChangeInPosition() {
		return mouseChangeInPosition;
	}
	float InputDirector::GetMouseScrollX() {
		return mouseScroll.x;
	}
	float InputDirector::GetMouseScrollY() {
		return mouseScroll.y;
	}

	void InputDirector::UpdateInputs() {
		glfwPollEvents();
	}

	void InputDirector::ResetReleasedInput() {
		std::fill(std::begin(keysDown), std::end(keysDown), 0);
		std::fill(std::begin(keysUp), std::end(keysUp), 0);
		std::fill(std::begin(mouseButtonsUp), std::end(mouseButtonsUp), 0);
		std::fill(std::begin(mouseButtonsDown), std::end(mouseButtonsDown), 0);
		mouseChangeInPosition = glm::dvec2(0, 0);
	}

	void InputDirector::keyEvent(int key, int scancode, int action, int mods) {
		switch (action) {
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

	void InputDirector::mouseButtonEvent(int button, int action, int mods) {
		switch (action) {
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

	void InputDirector::mouseMoveEvent(double xoffset, double yoffset) {
		mousePosition = glm::dvec2(xoffset, yoffset);

		if (firstMouse)
		{
			mousePositionPrevious = mousePosition;
			firstMouse = false;
		}

		mouseChangeInPosition = mousePositionPrevious - mousePosition;
		mouseChangeInPosition.x *= -1; //coordinates are reversed on y axis (top left vs bottom left)

		mousePositionPrevious = mousePosition;

		//glfwGetCursorPos(glfwGetCurrentContext(), &mousePosition.x, &mousePosition.y);
	}

	void InputDirector::mouseScrollEvent(double xoffset, double yoffset) {
		mouseScroll = glm::dvec2(xoffset, yoffset);
	}
}