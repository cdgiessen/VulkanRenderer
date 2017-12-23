
#include "VulkanApp.h"

#define STB_IMAGE_IMPLEMENTATION
#include "..\..\third-party\stb_image\stb_image.h"

#define VMA_IMPLEMENTATION
#include "..\..\third-party\VulkanMemoryAllocator\vk_mem_alloc.h"

void inline WaitForUserInput() {
	std::cerr << "Press anything to continue..." << std::endl;
	std::cin.get();
}

int main() {
	{
		std::shared_ptr<VulkanApp> vkApp;
		try {
			vkApp = std::make_shared<VulkanApp>();
		}
		catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			WaitForUserInput();
			return EXIT_FAILURE;
		}
	}
	
	WaitForUserInput();
	return EXIT_SUCCESS;

}