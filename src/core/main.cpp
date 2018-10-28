
#include <iostream>
#include <string>

#include "CoreTools.h"
#include "Logger.h"
#include "VulkanApp.h"

#define VMA_IMPLEMENTATION
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

//#define TINYGLTF_IMPLEMENTATION //needs stb_image & stb_image_write to be defined above
//#include "tinygltf/tiny_gltf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

int main (int argc, char* argv[])
{

	SetExecutableFilePath (argv[0]);

	std::unique_ptr<VulkanApp> vkApp;
	try
	{
		vkApp = std::make_unique<VulkanApp> ();
	}
	catch (const std::runtime_error& e)
	{
		Log::Error << "ENGINE FAILED TO INITIALIZE\n" << std::string (e.what ()) << "\n";
		return EXIT_FAILURE;
	}

	try
	{
		vkApp->Run ();
	}
	catch (const std::runtime_error& e)
	{
		Log::Error << "ENGINE FAILED IN MAIN LOOP\n" << std::string (e.what ()) << "\n";
		return EXIT_FAILURE;
	}

	try
	{
		vkApp.reset ();
	}
	catch (const std::runtime_error& e)
	{
		Log::Error << "ENGINE FAILED TO DESCRUCT\n" << std::string (e.what ()) << "\n";
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}