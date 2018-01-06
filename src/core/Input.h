#pragma once

//Input wrapper around GLFW, so the input isn't being driven by the vulkanapp class
#include <glm/common.hpp>
#include <GLFW/glfw3.h>

#include <array>


namespace Input {
	bool GetKey(int GLFW_KEY_CODE);
	bool GetKeyDown(int GLFW_KEY_CODE);
	bool GetKeyUp(int GLFW_KEY_CODE);

	bool GetMouseButton(int button);
	bool GetMouseButtonPressed(int button);
	bool GetMouseButtonReleased(int button);
	glm::vec2 GetMousePosition();
	glm::vec2 GetMouseChangeInPosition();
	float GetMouseScrollX();
	float GetMouseScrollY();
}

class InputDirector {
public:

	static InputDirector& GetInstance() {
		static InputDirector *instance = new InputDirector();
		return *instance;
	};

	bool GetKey(int GLFW_KEY_CODE);
	bool GetKeyDown(int GLFW_KEY_CODE);
	bool GetKeyUp(int GLFW_KEY_CODE);

	bool GetMouseButton(int button);
	bool GetMouseButtonPressed(int button);
	bool GetMouseButtonReleased(int button);
	glm::vec2 GetMousePosition();
	glm::vec2 GetMouseChangeInPosition();
	float GetMouseScrollX();
	float GetMouseScrollY();

	void keyEvent(int key, int scancode, int action, int mods);
	void mouseButtonEvent(int button, int action, int mods);
	void mouseMoveEvent(double xoffset, double yoffset);
	void mouseScrollEvent(double xoffset, double yoffset);

	void ResetReleasedInput();
	void UpdateInputs();

private:
	std::array<bool, 1024> keys = {{ false }};
	std::array<bool, 1024> keysDown = {{ false }};
	std::array<bool, 1024> keysUp = {{ false }};

	std::array<bool, 15> mouseButtons = {{ false }};
	std::array<bool, 15> mouseButtonsDown = {{ false }};
	std::array<bool, 15> mouseButtonsUp = {{ false }};

	bool firstMouse = true; //on start
	glm::vec2 mousePosition = glm::vec2(0,0);
	glm::vec2 mousePositionPrevious = glm::vec2(0,0);
	glm::vec2 mouseChangeInPosition = glm::vec2(0,0);
	glm::vec2 mouseScroll = glm::vec2(0, 0);

	InputDirector();

	static InputDirector* s_instance;

};
