#include "Window.h"

#include <climits>
#include <mutex>

#include "cml/cml.h"
#include <GLFW/glfw3.h>
#include "imgui.hpp"

#include "rendering/backend/ImGuiImplGLFW.h"

#include "Input.h"
#include "Logger.h"

void InitializeGLFW () { glfwInit (); }
void TerminateGLFW () { glfwTerminate (); }

Window::Window (Input::InputDirector& input,
    bool isFullscreen,
    const char* window_title,
    const cml::vec2i& size,
    const cml::vec2i& position = { 0, 0 })
: input (input)
{
	if (window_title == nullptr) window_title = "";
	glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
	if (isFullscreen)
	{
		GLFWmonitor* primary = glfwGetPrimaryMonitor ();
		const GLFWvidmode* mode = glfwGetVideoMode (primary);

		window = glfwCreateWindow (mode->width, mode->height, window_title, primary, NULL);
		Log.Debug (fmt::format ("Monitor Width {}", mode->width));
		Log.Debug (fmt::format ("Monitor Height {}", mode->height));
	}
	else
	{
		window = glfwCreateWindow (size.x, size.y, window_title, NULL, NULL);
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
	window = nullptr;
}

void Window::SetSizeLimits (const cml::vec2i& minSize, const cml::vec2i& maxSize = {})
{
	glfwSetWindowSizeLimits (window,
	    minSize.x,
	    minSize.y,
	    maxSize.x ? maxSize.x : minSize.x,
	    maxSize.y ? maxSize.y : minSize.y);
}

void Window::ShowWindow (bool show)
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

GLFWwindow* Window::GetWindowContext () const { return window; }

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
	Log.Error (fmt::format ("Error Code:{}, {}", error, description));
}

void Window::KeyboardHandler (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	w->input.keyEvent (key, scancode, action, mods);
	ImGui_ImplGlfw_KeyCallback (window, key, scancode, action, mods);
}

void Window::CharInputHandler (GLFWwindow* window, uint32_t codePoint)
{

	ImGui_ImplGlfw_CharCallback (window, codePoint);
}

void Window::MouseButtonHandler (GLFWwindow* window, int button, int action, int mods)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	w->input.mouseButtonEvent (button, action, mods);
	ImGui_ImplGlfw_MouseButtonCallback (window, button, action, mods);
}

void Window::MouseMoveHandler (GLFWwindow* window, double posx, double posy)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	w->input.mouseMoveEvent (posx, posy);
}

void Window::MouseScrollHandler (GLFWwindow* window, double xoffset, double yoffset)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	w->input.mouseScrollEvent (xoffset, yoffset);
	ImGui_ImplGlfw_ScrollCallback (window, xoffset, yoffset);
}

void Window::JoystickConfigurationChangeHandler (int joy, int event)
{
	if (event == GLFW_CONNECTED)
	{
		Log.Debug (fmt::format ("Controller {} Connected", joy));
		Input::InputDirector::ConnectJoystick (joy);
	}
	else if (event == GLFW_DISCONNECTED)
	{
		Log.Debug (fmt::format ("Controller {} Disconnected", joy));
		Input::InputDirector::DisconnectJoystick (joy);
	}
}

void Window::WindowCloseHandler (GLFWwindow* window)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	w->SetWindowToClose ();
}

void Window::FramebufferSizeHandler (GLFWwindow* window, int width, int height)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));

	w->updateWindowSize = true;
}

void Window::WindowResizeHandler (GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0) return;

	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	// glfwSetWindowSize(window, width, height);
	w->updateWindowSize = true;
}

void Window::WindowIconifyHandler (GLFWwindow* window, int iconified)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
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
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	if (focused)
	{
		w->isWindowFocused = true;
	}
	else
	{
		w->isWindowFocused = false;
	}
}