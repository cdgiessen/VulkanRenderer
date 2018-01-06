
#include <string>

#include "CoreTools.h"
#include "VulkanApp.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../third-party/stb_image/stb_image.h"

#define VMA_IMPLEMENTATION
#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

int main(int argc, char* argv[]) {

	SetExecutableFilePath(argv[0]);

	std::shared_ptr<VulkanApp> vkApp;
	try {
		vkApp = std::make_shared<VulkanApp>();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;

}