cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)

project(VulkanRenderer VERSION 0.0.1 LANGUAGES CXX)

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

#include(cotire)

find_package(glfw3 REQUIRED)

find_package(glm REQUIRED)

find_package(Vulkan REQUIRED)

#gtk include for linux
if(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")

	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

	include_directories(${GTK3_INCLUDE_DIRS})
	link_directories(${GTK3_LIBRARY_DIRS})
	add_definitions(${GTK3_CFLAGS_OTHER}) 
endif(UNIX)

add_executable(VulkanApp 

src/core/main.cpp
src/core/CoreTools.cpp
src/core/VulkanApp.cpp
src/core/Gradient.cpp
src/core/Input.cpp
src/core/Logger.cpp
src/core/MemoryPool.cpp
src/core/TimeManager.cpp
src/core/Window.cpp

src/gui/ImGuiImpl.cpp
src/gui/NodeGraph.cpp
src/gui/TerGenNodeGraph.cpp
src/gui/ProcTerrainNodeGraph.cpp
src/gui/InternalGraph.cpp

src/rendering/VulkanBuffer.cpp
src/rendering/VulkanDevice.cpp
src/rendering/VulkanDescriptor.cpp
src/rendering/VulkanPipeline.cpp
src/rendering/VulkanModel.cpp
src/rendering/VulkanRenderer.cpp
src/rendering/VulkanShader.cpp
src/rendering/VulkanSwapChain.cpp
src/rendering/VulkanTexture.cpp
src/rendering/VulkanTools.cpp

src/resources/Mesh.cpp
src/resources/MeshManager.cpp
src/resources/ResourceManager.cpp
src/resources/Texture.cpp
src/resources/TextureManager.cpp

src/scene/Camera.cpp
src/scene/GameObject.cpp
src/scene/InstancedSceneObject.cpp
src/scene/Scene.cpp
src/scene/Skybox.cpp
src/scene/Terrain.cpp
src/scene/TerrainGenerator.cpp
src/scene/TerrainManager.cpp
src/scene/Water.cpp

third-party/ImGui/imgui.cpp
third-party/ImGui/imgui_draw.cpp

third-party/FastNoiseSIMD/FastNoiseSIMD.cpp
third-party/FastNoiseSIMD/FastNoiseSIMD_avx2.cpp
third-party/FastNoiseSIMD/FastNoiseSIMD_avx512.cpp
third-party/FastNoiseSIMD/FastNoiseSIMD_internal.cpp
third-party/FastNoiseSIMD/FastNoiseSIMD_neon.cpp
third-party/FastNoiseSIMD/FastNoiseSIMD_sse2.cpp
third-party/FastNoiseSIMD/FastNoiseSIMD_sse41.cpp
)

#glm
#include_directories(glm)
target_include_directories(VulkanApp PUBLIC glm)

#glfw
target_include_directories(VulkanApp PUBLIC glfw)
target_link_libraries(VulkanApp PRIVATE glfw)

#vulkan
if(VULKAN_FOUND)
	target_include_directories(VulkanApp PRIVATE ${Vulkan_INCLUDE_DIRS})
	target_link_libraries(VulkanApp PRIVATE ${Vulkan_LIBRARIES})
endif (VULKAN_FOUND)

#gtk
if(UNIX)
	target_link_libraries(VulkanApp ${GTK3_LIBRARIES})
endif(UNIX)

#set_target_properties(VulkanApp PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
#cotire(VulkanApp)

if(MSVC)
	add_compile_options("/permissive-")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")	
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
endif(MSVC)

if (UNIX)
add_custom_command(TARGET VulkanApp POST_BUILD        # Adds a post-build event to VulkanApp
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${PROJECT_BINARY_DIR}/third-party/glfw-3.2.1/src/libglfw.so"      # <--this is in-file
 "${PROJECT_BINARY_DIR}")										# <--this is out-file path

endif(UNIX)

# copy settings.json
add_custom_command(TARGET VulkanApp PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${PROJECT_SOURCE_DIR}/settings.json $<TARGET_FILE_DIR:VulkanApp>/settings.json)


# copy files over
add_custom_command(TARGET VulkanApp PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:VulkanApp>/assets)
