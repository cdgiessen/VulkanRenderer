#pragma once

#include <string>
#include <vector>
#include <functional>
#include <set>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

namespace glfw {
	std::set<std::string> getRequiredInstanceExtensions();
	VkSurfaceKHR createWindowSurface(const vk::Instance& instance, GLFWwindow* window, const vk::AllocationCallbacks* pAllocator = nullptr);

	class Window {
	protected:
		Window();

		virtual void createWindow(const glm::uvec2& size, const glm::ivec2& position = { INT_MIN, INT_MIN });
		void showWindow(bool show = true);
		void setSizeLimits(const glm::uvec2& minSize, const glm::uvec2& maxSize = {});
		virtual void prepareWindow();
		virtual void destroyWindow();
		void runWindowLoop(const std::function<void()>& frameHandler);

		//
		// Event handlers are called by the GLFW callback mechanism and should not be called directly
		//

		virtual void windowResized(const glm::uvec2& newSize) { }
		virtual void windowClosed() { }

		// Keyboard handling
		virtual void keyEvent(int key, int scancode, int action, int mods) {
			switch (action) {
			case GLFW_PRESS:
				keyPressed(key, mods);
				break;

			case GLFW_RELEASE:
				keyReleased(key, mods);
				break;

			default:
				break;
			}
		}
		virtual void keyPressed(int key, int mods) { }
		virtual void keyReleased(int key, int mods) { }

		// Mouse handling 
		virtual void mouseButtonEvent(int button, int action, int mods) {
			switch (action) {
			case GLFW_PRESS:
				mousePressed(button, mods);
				break;

			case GLFW_RELEASE:
				mouseReleased(button, mods);
				break;

			default:
				break;
			}
		}
		virtual void mousePressed(int button, int mods) { }
		virtual void mouseReleased(int button, int mods) { }
		virtual void mouseMoved(const glm::vec2& newPos) { }
		virtual void mouseScrolled(float delta) { }

		static void KeyboardHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void MouseButtonHandler(GLFWwindow* window, int button, int action, int mods);
		static void MouseMoveHandler(GLFWwindow* window, double posx, double posy);
		static void MouseScrollHandler(GLFWwindow* window, double xoffset, double yoffset);
		static void CloseHandler(GLFWwindow* window);
		static void FramebufferSizeHandler(GLFWwindow* window, int width, int height);

		GLFWwindow* window{ nullptr };
	};

}