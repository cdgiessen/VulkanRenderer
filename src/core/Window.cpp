#include "Window.h"

#include <climits>
#include <mutex>

#include <GLFW/glfw3.h>

#include "cml/cml.h"
#include "gui/ImGuiImpl.h"
#include "imgui.hpp"

#include "Input.h"
#include "Logger.h"


Window::Window (bool isFullscreen, const cml::vec2i& size, const cml::vec2i& position = { 0, 0 })
{
	glfwInit ();
	glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
	if (isFullscreen)
	{
		GLFWmonitor* primary = glfwGetPrimaryMonitor ();
		const GLFWvidmode* mode = glfwGetVideoMode (primary);

		window = glfwCreateWindow (mode->width, mode->height, "Vulkan Renderer", primary, NULL);
		Log.Debug (fmt::format ("Monitor Width {}\n", mode->width));
		Log.Debug (fmt::format ("Monitor Height {}\n", mode->height));
		// Log.Debug << "Monitor Width " << mode->width << "\n";
		// Log.Debug << "Monitor Height " << mode->height << "\n";
	}
	else
	{
		window = glfwCreateWindow (size.x, size.y, "Vulkan Renderer", NULL, NULL);
		if (position != cml::vec2i{ INT_MIN, INT_MIN })
		{
			glfwSetWindowPos (window, position.x, position.y);
		}
	}

	// Prepare window
	glfwSetWindowUserPointer (window, this);
	glfwSetFramebufferSizeCallback (window, FramebufferSizeHandler);
	glfwSetWindowSizeCallback (window, WindowResizeHandler);
	glfwSetWindowIconifyCallback (window, WindowIconifyHandler);
	glfwSetWindowFocusCallback (window, WindowFocusHandler);
	glfwSetWindowCloseCallback (window, WindowCloseHandler);
	glfwSetErrorCallback (ErrorHandler);

	// Set input callbacks
	glfwSetKeyCallback (window, KeyboardHandler);
	glfwSetCharCallback (window, CharInputHandler);
	glfwSetMouseButtonCallback (window, MouseButtonHandler);
	glfwSetCursorPosCallback (window, MouseMoveHandler);
	glfwSetScrollCallback (window, MouseScrollHandler);
	glfwSetJoystickCallback (JoystickConfigurationChangeHandler);

	isWindowIconofied = glfwGetWindowAttrib (window, GLFW_ICONIFIED);
	isWindowFocused = glfwGetWindowAttrib (window, GLFW_FOCUSED);

	currentWindowSize = GetWindowSize ();
}

Window::~Window ()
{
	glfwDestroyWindow (window);
	glfwTerminate ();
	window = nullptr;
}

void Window::setSizeLimits (const cml::vec2i& minSize, const cml::vec2i& maxSize = {})
{
	glfwSetWindowSizeLimits (window,
	    minSize.x,
	    minSize.y,
	    maxSize.x ? maxSize.x : minSize.x,
	    maxSize.y ? maxSize.y : minSize.y);
}

void Window::showWindow (bool show)
{
	if (show)
	{
		glfwShowWindow (window);
	}
	else
	{
		glfwHideWindow (window);
	}
}

GLFWwindow* Window::getWindowContext () const { return window; }

bool Window::CheckForWindowResizing () { return updateWindowSize; }

void Window::SetWindowResizeDone () { updateWindowSize = false; }


bool Window::CheckForWindowIconified () { return isWindowIconofied; }

bool Window::CheckForWindowFocus () { return isWindowFocused; }

bool Window::CheckForWindowClose () { return shouldCloseWindow; }

void Window::SetWindowToClose ()
{
	shouldCloseWindow = true;
	glfwSetWindowShouldClose (window, true);
}

cml::vec2i Window::GetWindowSize ()
{
	int width, height;
	glfwGetWindowSize (window, &width, &height);
	return cml::vec2i (width, height);
}

void Window::ErrorHandler (int error, const char* description)
{
	Log.Error (fmt::format ("Error Code:{}, {}\n", error, description));
}

void Window::KeyboardHandler (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Input::inputDirector.keyEvent (key, scancode, action, mods);
	ImGui_ImplGlfwVulkan_KeyCallback (window, key, scancode, action, mods);
}

void Window::CharInputHandler (GLFWwindow* window, uint32_t codePoint)
{

	ImGui_ImplGlfwVulkan_CharCallback (window, codePoint);
}

void Window::MouseButtonHandler (GLFWwindow* window, int button, int action, int mods)
{
	Input::inputDirector.mouseButtonEvent (button, action, mods);
	ImGui_ImplGlfwVulkan_MouseButtonCallback (window, button, action, mods);
}

void Window::MouseMoveHandler (GLFWwindow* window, double posx, double posy)
{
	Input::inputDirector.mouseMoveEvent (posx, posy);
}

void Window::MouseScrollHandler (GLFWwindow* window, double xoffset, double yoffset)
{
	Input::inputDirector.mouseScrollEvent (xoffset, yoffset);
	ImGui_ImplGlfwVulkan_ScrollCallback (window, xoffset, yoffset);
}

void Window::JoystickConfigurationChangeHandler (int joy, int event)
{
	if (event == GLFW_CONNECTED)
	{
		Log.Debug (fmt::format ("Controller {} Connected\n", joy));
		// Log.Debug << "Controller " << joy << " Connected \n";
		Input::ConnectJoystick (joy);
	}
	else if (event == GLFW_DISCONNECTED)
	{
		Log.Debug (fmt::format ("Controller {} Disconnected\n", joy));
		// Log.Debug << "Controller " << joy << " Disconnected \n";
		Input::DisconnectJoystick (joy);
	}
}

void Window::WindowCloseHandler (GLFWwindow* window)
{
	Window* w = (Window*)glfwGetWindowUserPointer (window);
	w->SetWindowToClose ();
}

void Window::FramebufferSizeHandler (GLFWwindow* window, int width, int height)
{
	Window* w = (Window*)glfwGetWindowUserPointer (window);

	w->updateWindowSize = true;
}

void Window::WindowResizeHandler (GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0) return;

	Window* w = (Window*)glfwGetWindowUserPointer (window);
	// glfwSetWindowSize(window, width, height);
	w->updateWindowSize = true;
}

void Window::WindowIconifyHandler (GLFWwindow* window, int iconified)
{

	Window* w = (Window*)glfwGetWindowUserPointer (window);
	if (iconified)
	{
		w->isWindowIconofied = true;
	}
	else
	{
		w->isWindowIconofied = false;
	}
}

void Window::WindowFocusHandler (GLFWwindow* window, int focused)
{
	Window* w = (Window*)glfwGetWindowUserPointer (window);
	if (focused)
	{
		w->isWindowFocused = true;
	}
	else
	{
		w->isWindowFocused = false;
	}
}

std::vector<const char*> GetWindowExtensions ()
{

	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions (&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++)
	{
		extensions.push_back (glfwExtensions[i]);
	}

	return extensions;
}