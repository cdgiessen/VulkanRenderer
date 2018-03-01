#pragma once

#include <string>
#include <vector>
#include <set>
#include <vulkan/vulkan.hpp>
#include <glm/vec2.hpp>
#include <GLFW/glfw3.h>

  
  

std::set<std::string> getRequiredInstanceExtensions();
VkSurfaceKHR createWindowSurface(const vk::Instance& instance, GLFWwindow* window, const vk::AllocationCallbacks* pAllocator = nullptr);

struct Destroyer {

	~Destroyer() {
		glfwTerminate();
	}
};

static std::shared_ptr<Destroyer> DESTROYER;

class Window {
public:
	Window();                                                                                                                                                                                            

	void createWindow(const glm::uvec2& size, const glm::ivec2& position = { 0, 0 });
	void showWindow(bool show = true);
	void setSizeLimits(const glm::uvec2& minSize, const glm::uvec2& maxSize = {});
	void prepareWindow();
	void destroyWindow();

	GLFWwindow* getWindowContext();
	bool CheckForWindowResizing();
	void SetWindowResizeDone();
	bool CheckForWindowClose();
	void SetWindowToClose();

protected:
	//
	// Event handlers are called by the GLFW callback mechanism and should not be called directly
	//

	static void KeyboardHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void CharInputHandler(GLFWwindow* window, uint32_t codePoint);
	static void MouseButtonHandler(GLFWwindow* window, int button, int action, int mods);
	static void MouseMoveHandler(GLFWwindow* window, double posx, double posy);
	static void MouseScrollHandler(GLFWwindow* window, double xoffset, double yoffset);

	static void FramebufferSizeHandler(GLFWwindow* window, int width, int height);
	static void WindowResizeHandler(GLFWwindow* window, int width, int height);
	static void CloseHandler(GLFWwindow* window);

	GLFWwindow* window{ nullptr };
	bool updateWindowSize = false;
	bool shouldCloseWindow = false;
};

