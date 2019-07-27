#pragma once

#include <vector>

#include "cml/cml.h"

struct GLFWwindow;

class Window
{
	public:
	Window (bool isFullscreen, const cml::vec2i& size, const cml::vec2i& position);
	~Window ();

	// void createWindow(bool isFullscreen, const cml::vec2i& size, const cml::vec2i& position = { 0, 0 });
	void showWindow (bool show = true);
	void setSizeLimits (const cml::vec2i& minSize, const cml::vec2i& maxSize);

	GLFWwindow* getWindowContext () const;
	bool CheckForWindowResizing ();
	void SetWindowResizeDone ();
	bool CheckForWindowClose ();
	void SetWindowToClose ();

	bool CheckForWindowIconified ();
	bool CheckForWindowFocus ();

	cml::vec2i GetWindowSize ();

	protected:
	//
	// Event handlers are called by the GLFW callback mechanism and should not be called directly
	//
	static void ErrorHandler (int error, const char* description);

	static void KeyboardHandler (GLFWwindow* window, int key, int scancode, int action, int mods);
	static void CharInputHandler (GLFWwindow* window, uint32_t codePoint);
	static void MouseButtonHandler (GLFWwindow* window, int button, int action, int mods);
	static void MouseMoveHandler (GLFWwindow* window, double posx, double posy);
	static void MouseScrollHandler (GLFWwindow* window, double xoffset, double yoffset);
	static void JoystickConfigurationChangeHandler (int joy, int event);
	static void FramebufferSizeHandler (GLFWwindow* window, int width, int height);
	static void WindowResizeHandler (GLFWwindow* window, int width, int height);
	static void WindowFocusHandler (GLFWwindow* window, int focused);
	static void WindowIconifyHandler (GLFWwindow* window, int iconified);
	static void WindowCloseHandler (GLFWwindow* window);

	cml::vec2i currentWindowSize;

	GLFWwindow* window{ nullptr };
	bool updateWindowSize = false;
	bool shouldCloseWindow = false;
	bool isWindowIconofied = false;
	bool isWindowFocused = false;
};

std::vector<const char*> GetWindowExtensions ();