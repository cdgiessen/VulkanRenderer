#include "Input.h"

#include <algorithm>

//External facing access to buttons. 
namespace Input {

	bool GetKey(int GLFW_KEY_CODE) {
		return InputDirector::GetInstance().GetKey(GLFW_KEY_CODE);
	}

	bool GetKeyDown(int GLFW_KEY_CODE) {
		return InputDirector::GetInstance().GetKeyDown(GLFW_KEY_CODE);
	}

	bool GetKeyUp(int GLFW_KEY_CODE) {
		return InputDirector::GetInstance().GetKeyUp(GLFW_KEY_CODE);
	}

	bool GetMouseButton(int button) {
		return InputDirector::GetInstance().GetMouseButton(button);
	}
	bool GetMouseButtonPressed(int button) {
		return InputDirector::GetInstance().GetMouseButtonPressed(button);
	}
	bool GetMouseButtonReleased(int button) {
		return InputDirector::GetInstance().GetMouseButtonReleased(button);
	}
	glm::vec2 GetMousePosition() {
		return InputDirector::GetInstance().GetMousePosition();
	}
	glm::vec2 GetMouseChangeInPosition() {
		return InputDirector::GetInstance().GetMouseChangeInPosition();
	}
	float GetMouseScroll() {
		return InputDirector::GetInstance().GetMouseScroll();
	}

}

InputDirector::InputDirector() {

}

bool InputDirector::GetKey(int GLFW_KEY_CODE) {
	return keys[GLFW_KEY_CODE];
}

bool InputDirector::GetKeyDown(int GLFW_KEY_CODE) {
	return keysDown[GLFW_KEY_CODE];
}

bool InputDirector::GetKeyUp(int GLFW_KEY_CODE) {
	return keysUp[GLFW_KEY_CODE];
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
float InputDirector::GetMouseScroll() {
	return mouseScroll.x;
}

void InputDirector::UpdateInputs() {
	glfwPollEvents();


}

void InputDirector::ResetReleasedInput() {
	std::fill(std::begin(keysUp), std::end(keysUp), 0);
	std::fill(std::begin(mouseButtonsUp), std::end(mouseButtonsUp), 0);
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


