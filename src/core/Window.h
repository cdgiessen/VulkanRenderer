#pragma once

#include <glm/glm.hpp>
#include <vector>

struct GLFWwindow;

class Window {
public:
	Window(bool isFullscreen, const glm::ivec2& size, const glm::ivec2& position);
	~Window();

	//void createWindow(bool isFullscreen, const glm::ivec2& size, const glm::ivec2& position = { 0, 0 });
	void showWindow(bool show = true);
	void setSizeLimits(const glm::ivec2& minSize, const glm::ivec2& maxSize);

	GLFWwindow* getWindowContext() const;
	bool CheckForWindowResizing();
	void SetWindowResizeDone();
	bool CheckForWindowClose();
	void SetWindowToClose();

	bool CheckForWindowIconified();
	bool CheckForWindowFocus();

	glm::ivec2 GetWindowSize();

protected:
	//
	// Event handlers are called by the GLFW callback mechanism and should not be called directly
	//
	static void ErrorHandler(int error, const char* description);

	static void KeyboardHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void CharInputHandler(GLFWwindow* window, uint32_t codePoint);
	static void MouseButtonHandler(GLFWwindow* window, int button, int action, int mods);
	static void MouseMoveHandler(GLFWwindow* window, double posx, double posy);
	static void MouseScrollHandler(GLFWwindow* window, double xoffset, double yoffset);
	static void JoystickConfigurationChangeHandler(int joy, int event);
	static void FramebufferSizeHandler(GLFWwindow* window, int width, int height);
	static void WindowResizeHandler(GLFWwindow* window, int width, int height);
	static void WindowFocusHandler(GLFWwindow* window, int focused);
	static void WindowIconifyHandler(GLFWwindow* window, int iconified);
	static void WindowCloseHandler(GLFWwindow* window);

	glm::ivec2 currentWindowSize;

	GLFWwindow* window{ nullptr };
	bool updateWindowSize = false;
	bool shouldCloseWindow = false;
	bool isWindowIconofied = false;
	bool isWindowFocused = false;
};

std::vector<const char*> GetWindowExtensions();