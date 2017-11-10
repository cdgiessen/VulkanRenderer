#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
#define DBG_NEW new
#endif

#include "VulkanApp.h"

int main() {
	{
		VulkanApp* vkApp;
		try {
			vkApp = new VulkanApp();
		}
		catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			system("PAUSE");
			return EXIT_FAILURE;
		}
	}
	_CrtDumpMemoryLeaks();
	system("PAUSE");
	return EXIT_SUCCESS;

}