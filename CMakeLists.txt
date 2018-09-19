cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

include(CMakeToolsHelpers OPTIONAL)

set(CMAKE_CXX_STANDARD 17)
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)


#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
#set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#include(cotire)

project(VulkanRenderer VERSION 0.0.1 LANGUAGES CXX)


#libraries

find_package(glfw3 REQUIRED)

find_package(glm REQUIRED)

find_package(Vulkan REQUIRED)

find_package(Bullet) 

#gtk include for linux file window
if(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")

	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

	include_directories(${GTK3_INCLUDE_DIRS})
	link_directories(${GTK3_LIBRARY_DIRS})
	add_definitions(${GTK3_CFLAGS_OTHER}) 
endif(UNIX)

#memory

#set(FOONATHAN_MEMORY_BUILD_EXAMPLES OFF)
#set(FOONATHAN_MEMORY_BUILD_TESTS OFF)
#set(FOONATHAN_MEMORY_BUILD_TOOLS OFF)

#add_subdirectory(ext/memory)

# main files to compile

add_executable(VulkanApp 

src/core/main.cpp
src/core/CoreTools.cpp
src/core/VulkanApp.cpp
src/core/JobSystem.cpp
src/core/Input.cpp
src/core/Logger.cpp
src/core/TimeManager.cpp
src/core/Window.cpp

src/util/Gradient.cpp

src/gui/ImGuiImpl.cpp
src/gui/ProcTerrainNodeGraph.cpp
src/gui/InternalGraph.cpp

src/rendering/Buffer.cpp
src/rendering/Device.cpp
src/rendering/Descriptor.cpp
src/rendering/Material.cpp
src/rendering/Model.cpp
src/rendering/Pipeline.cpp
src/rendering/Renderer.cpp
src/rendering/FrameGraph.cpp
src/rendering/RenderTools.cpp
src/rendering/Shader.cpp
src/rendering/SwapChain.cpp
src/rendering/Texture.cpp
src/rendering/Wrappers.cpp

src/resources/Material.cpp
src/resources/Mesh.cpp
src/resources/Texture.cpp
src/resources/ResourceManager.cpp

src/scene/Camera.cpp
src/scene/GameObject.cpp
src/scene/InstancedSceneObject.cpp
src/scene/Scene.cpp
src/scene/Skybox.cpp
src/scene/Terrain.cpp
src/scene/TerrainManager.cpp
src/scene/Transform.cpp

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


#json
target_include_directories(VulkanApp PUBLIC third-party/json)

#glm
target_include_directories(VulkanApp PUBLIC glm)
target_link_libraries(VulkanApp PUBLIC glm)

#glfw
target_include_directories(VulkanApp PUBLIC glfw)
target_link_libraries(VulkanApp PRIVATE glfw)

#vulkan
if(VULKAN_FOUND)
	target_include_directories(VulkanApp PRIVATE ${Vulkan_INCLUDE_DIRS})
	target_link_libraries(VulkanApp PRIVATE ${Vulkan_LIBRARIES})
endif (VULKAN_FOUND)

#bullet physics
if(BULLET_FOUND)
	target_include_directories(VulkanApp PRIVATE ${BULLET_INCLUDE_DIRS})
	target_link_libraries(VulkanApp PRIVATE ${BULLET_LIBRARIES})
endif(BULLET_FOUND)

#gtk
if(UNIX)
	target_link_libraries(VulkanApp PRIVATE ${GTK3_LIBRARIES})
endif(UNIX)

#memory
#target_link_libraries(VulkanApp PUBLIC foonathan_memory)

#set_target_properties(VulkanApp PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
#cotire(VulkanApp)

if(MSVC)
	add_compile_options("/permissive-")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
	add_compile_options("/Debug:fastlink")
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Debug:fastlink")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")	
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
endif(MSVC)


# copy files over
add_custom_command(TARGET VulkanApp POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:VulkanApp>/assets)
