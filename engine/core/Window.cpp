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
		Log.debug (fmt::format ("Monitor Width {}", mode->width));
		Log.debug (fmt::format ("Monitor Height {}", mode->height));
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
	glfwSetFramebufferSizeCallback (window, framebuffer_size_handler);
	glfwSetWindowSizeCallback (window, window_resize_handler);
	glfwSetWindowIconifyCallback (window, window_iconify_handler);
	glfwSetWindowFocusCallback (window, window_focus_handler);
	glfwSetWindowCloseCallback (window, window_close_handler);
	glfwSetErrorCallback (error_handler);

	// Set input callbacks
	glfwSetKeyCallback (window, keyboard_handler);
	glfwSetCharCallback (window, char_input_handler);
	glfwSetMouseButtonCallback (window, mouse_button_handler);
	glfwSetCursorPosCallback (window, mouse_move_handler);
	glfwSetScrollCallback (window, mouse_scroll_handler);
	glfwSetJoystickCallback (joystick_configuration_change_handler);

	isWindowIconofied = glfwGetWindowAttrib (window, GLFW_ICONIFIED);
	isWindowFocused = glfwGetWindowAttrib (window, GLFW_FOCUSED);

	current_window_size = get_window_size ();
}

Window::~Window ()
{
	glfwDestroyWindow (window);
	window = nullptr;
}

void Window::set_size_limits (const cml::vec2i& minSize, const cml::vec2i& maxSize = {})
{
	glfwSetWindowSizeLimits (window,
	    minSize.x,
	    minSize.y,
	    maxSize.x ? maxSize.x : minSize.x,
	    maxSize.y ? maxSize.y : minSize.y);
}

void Window::show_window (bool show)
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

GLFWwindow* Window::get_window_context () const { return window; }

bool Window::should_window_resize () { return updateWindowSize; }

void Window::set_window_resize_done () { updateWindowSize = false; }


bool Window::is_window_iconified () { return isWindowIconofied; }

bool Window::is_window_focused () { return isWindowFocused; }

bool Window::should_window_close () { return shouldCloseWindow; }

void Window::set_window_close ()
{
	shouldCloseWindow = true;
	glfwSetWindowShouldClose (window, true);
}

cml::vec2i Window::get_window_size ()
{
	int width, height;
	glfwGetWindowSize (window, &width, &height);
	return cml::vec2i (width, height);
}

void Window::error_handler (int error, const char* description)
{
	Log.error (fmt::format ("Error Code:{}, {}", error, description));
}

void Window::keyboard_handler (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	w->input.key_event (key, scancode, action, mods);
	ImGui_ImplGlfw_KeyCallback (window, key, scancode, action, mods);
}

void Window::char_input_handler (GLFWwindow* window, uint32_t codePoint)
{

	ImGui_ImplGlfw_CharCallback (window, codePoint);
}

void Window::mouse_button_handler (GLFWwindow* window, int button, int action, int mods)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	w->input.mouse_button_event (button, action, mods);
	ImGui_ImplGlfw_MouseButtonCallback (window, button, action, mods);
}

void Window::mouse_move_handler (GLFWwindow* window, double posx, double posy)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	w->input.mouse_move_event (posx, posy);
}

void Window::mouse_scroll_handler (GLFWwindow* window, double xoffset, double yoffset)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	w->input.mouse_scroll_event (xoffset, yoffset);
	ImGui_ImplGlfw_ScrollCallback (window, xoffset, yoffset);
}

void Window::joystick_configuration_change_handler (int joy, int event)
{
	if (event == GLFW_CONNECTED)
	{
		Log.debug (fmt::format ("Controller {} Connected", joy));
		Input::InputDirector::connect_joystick (joy);
	}
	else if (event == GLFW_DISCONNECTED)
	{
		Log.debug (fmt::format ("Controller {} Disconnected", joy));
		Input::InputDirector::disconnect_joystick (joy);
	}
}

void Window::window_close_handler (GLFWwindow* window)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	w->set_window_close ();
}

void Window::framebuffer_size_handler (GLFWwindow* window, int width, int height)
{
	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));

	w->updateWindowSize = true;
}

void Window::window_resize_handler (GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0) return;

	Window* w = reinterpret_cast<Window*> (glfwGetWindowUserPointer (window));
	// glfwSetWindowSize(window, width, height);
	w->updateWindowSize = true;
}

void Window::window_iconify_handler (GLFWwindow* window, int iconified)
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

void Window::window_focus_handler (GLFWwindow* window, int focused)
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