#pragma once

#include "cml/cml.h"

struct GLFWwindow;

void InitializeGLFW ();
void TerminateGLFW ();

namespace Input
{
class InputDirector;
}
class Window
{
	public:
	Window (Input::InputDirector& input,
	    bool isFullscreen,
	    const char* window_title,
	    const cml::vec2i& size,
	    const cml::vec2i& position);
	~Window ();

	void show_window (bool show = true);
	void set_size_limits (const cml::vec2i& minSize, const cml::vec2i& maxSize);

	GLFWwindow* get_window_context () const;
	bool should_window_resize ();
	void set_window_resize_done ();
	bool should_window_close ();
	void set_window_close ();

	bool is_window_iconified ();
	bool is_window_focused ();

	cml::vec2i get_window_size ();

	private:
	Input::InputDirector& input;
	//
	// Event handlers are called by the GLFW callback mechanism and should not be called directly
	//
	static void error_handler (int error, const char* description);

	static void keyboard_handler (GLFWwindow* window, int key, int scancode, int action, int mods);
	static void char_input_handler (GLFWwindow* window, uint32_t codePoint);
	static void mouse_button_handler (GLFWwindow* window, int button, int action, int mods);
	static void mouse_move_handler (GLFWwindow* window, double posx, double posy);
	static void mouse_scroll_handler (GLFWwindow* window, double xoffset, double yoffset);
	static void joystick_configuration_change_handler (int joy, int event);
	static void framebuffer_size_handler (GLFWwindow* window, int width, int height);
	static void window_resize_handler (GLFWwindow* window, int width, int height);
	static void window_focus_handler (GLFWwindow* window, int focused);
	static void window_iconify_handler (GLFWwindow* window, int iconified);
	static void window_close_handler (GLFWwindow* window);

	cml::vec2i current_window_size;

	GLFWwindow* window{ nullptr };
	bool updateWindowSize = false;
	bool shouldCloseWindow = false;
	bool isWindowIconofied = false;
	bool isWindowFocused = false;
};
