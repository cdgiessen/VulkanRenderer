#include "Window.hpp"
#include <mutex>
#include "Input.h"

#include "..\third-party\ImGui\imgui.h"
#include "..\gui\ImGuiImpl.h"

std::set<std::string> getRequiredInstanceExtensions() {
	std::set<std::string> result;
	uint32_t count = 0;
	const char** names = glfwGetRequiredInstanceExtensions(&count);
	if (names && count) {
		for (uint32_t i = 0; i < count; ++i) {
			result.insert(names[i]);
		}
	}
	return result;
}

Window::Window() {
	static std::once_flag once;
	std::call_once(once, [] {
		if (GLFW_TRUE == glfwInit()) {
			DESTROYER.reset(new Destroyer());
		}
	});
}

void Window::createWindow(const glm::uvec2& size, const glm::ivec2& position) {
	glfwInit();
	window = glfwCreateWindow(size.x, size.y, "Vulkan Renderer", NULL, NULL);
	if (position != glm::ivec2{ INT_MIN, INT_MIN }) {
		glfwSetWindowPos(window, position.x, position.y);
	}
	prepareWindow();
}

void Window::setSizeLimits(const glm::uvec2& minSize, const glm::uvec2& maxSize) {
	glfwSetWindowSizeLimits(window, minSize.x, minSize.y, maxSize.x ? maxSize.x : minSize.x, maxSize.y ? maxSize.y : minSize.y);
}

void Window::showWindow(bool show) {
	if (show) {
		glfwShowWindow(window);
	}
	else {
		glfwHideWindow(window);
	}
}

void Window::prepareWindow() {
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeHandler);
	glfwSetWindowSizeCallback(window, WindowResizeHandler);
	glfwSetWindowCloseCallback(window, CloseHandler);

	glfwSetKeyCallback(window, KeyboardHandler);
	glfwSetMouseButtonCallback(window, MouseButtonHandler);
	glfwSetCursorPosCallback(window, MouseMoveHandler);
	glfwSetScrollCallback(window, MouseScrollHandler);
}

void Window::destroyWindow() {
	glfwDestroyWindow(window);
	window = nullptr;
}

GLFWwindow* Window::getWindowContext() {
	return window;
}

bool Window::CheckForWindowResizing() {
	return updateWindowSize;
}

void Window::SetWindowResizeDone() {
	updateWindowSize = false;
}

bool Window::CheckForWindowClose() {
	return shouldCloseWindow;
}

void Window::SetWindowToClose() {
	shouldCloseWindow = true;
	glfwSetWindowShouldClose(window, true);
}

VkSurfaceKHR createWindowSurface(const vk::Instance& instance, GLFWwindow* window, const vk::AllocationCallbacks* allocator) {
	VkSurfaceKHR rawSurface;
	vk::Result result = static_cast<vk::Result>(glfwCreateWindowSurface((VkInstance)instance, window, reinterpret_cast<const VkAllocationCallbacks*>(allocator), &rawSurface));
	return vk::createResultValue(result, rawSurface, "vk::CommandBuffer::begin");
}

void Window::KeyboardHandler(GLFWwindow* window, int key, int scancode, int action, int mods) {
	InputDirector::GetInstance().keyEvent(key, scancode, action, mods);
	ImGui_ImplGlfwVulkan_KeyCallback(window, key, scancode, action, mods);
}

void Window::MouseButtonHandler(GLFWwindow* window, int button, int action, int mods) {
	InputDirector::GetInstance().mouseButtonEvent(button, action, mods);
	ImGui_ImplGlfwVulkan_MouseButtonCallback(window, button, action, mods);
}

void Window::MouseMoveHandler(GLFWwindow* window, double posx, double posy) {
	InputDirector::GetInstance().mouseMoveEvent(posx, posy);
}

void Window::MouseScrollHandler(GLFWwindow* window, double xoffset, double yoffset) {
	InputDirector::GetInstance().mouseScrollEvent(xoffset, yoffset);
}

void Window::CloseHandler(GLFWwindow* window) {
	Window* w = (Window*)glfwGetWindowUserPointer(window);
	w->SetWindowToClose();
}

void Window::FramebufferSizeHandler(GLFWwindow* window, int width, int height) {
	Window* w = (Window*)glfwGetWindowUserPointer(window);

	w->updateWindowSize = true;
}

void Window::WindowResizeHandler(GLFWwindow* window, int width, int height) {
	if (width == 0 || height == 0) return;

	Window* w = (Window*)glfwGetWindowUserPointer(window);
	//glfwSetWindowSize(window, width, height);
	w->updateWindowSize = true;
	
}
