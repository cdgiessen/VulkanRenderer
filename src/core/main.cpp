
#include <string>
#include <iostream>

#include "CoreTools.h"
#include "VulkanApp.h"
#include "Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../third-party/stb_image/stb_image.h"

#define VMA_IMPLEMENTATION
#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

int main(int argc, char* argv[]) {

	SetExecutableFilePath(argv[0]);

	std::shared_ptr<VulkanApp> vkApp;
	try {
		vkApp = std::make_shared<VulkanApp>();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		Log::Error << std::string(e.what()) << "\n";

		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;

}