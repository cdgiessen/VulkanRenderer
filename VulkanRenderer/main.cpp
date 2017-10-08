#include "VulkanApp.h"

int main() {

	VulkanApp* vkApp;
	try {
		vkApp = new VulkanApp();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		system("PAUSE");
		return EXIT_FAILURE;
	}
	system("PAUSE");
	return EXIT_SUCCESS;

}