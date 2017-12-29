cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(VulkanRenderer VERSION 0.0.1 LANGUAGES CXX)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Vulkan REQUIRED)

find_package(glm REQUIRED)

#set(${GLFW_ROOT} "/GLFW")

#find_package(glfw REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")

add_executable(VulkanApp 

src/core/main.cpp
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

src/rendering/VulkanDevice.cpp
src/rendering/VulkanLargeBuffer.cpp
src/rendering/VulkanPipeline.cpp
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



#set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
#set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

#add_subdirectory(third-party/glfw-3.2.1)

#add_subdirectory(third-party/glm)

set(GLFW_DIR "third-party/glfw-3.2.1")

set(GLFW_BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build shared libraries")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")

add_subdirectory(third-party/glfw-3.2.1)

target_link_libraries(VulkanApp "glfw" "${GLFW_LIBRARY}")
target_link_libraries(VulkanApp glfw)
target_include_directories(VulkanApp PUBLIC "${GLFW_DIR}/include")

#glfw
if(GLFW_FOUND)
	#target_link_libraries(VulkanApp ${GLFW_LIBRARIES})
	#target_include_directories(VulkanApp PUBLIC ${GLFW_INCLUDE_DIR})
	target_link_libraries(VulkanApp "${GLFW_LIBRARIES}")
	target_include_directories(VulkanApp PRIVATE "${GLFW_DIR}/include")
endif(GLFW_FOUND)


#vulkan
if(VULKAN_FOUND)
	target_include_directories(VulkanApp PRIVATE ${Vulkan_INCLUDE_DIRS})
	target_link_libraries(VulkanApp ${Vulkan_LIBRARIES})
endif (VULKAN_FOUND)

#glm
if(GLM_FOUND)
	target_include_directories(VulkanApp PUBLIC third-party)
	include_directories(third-party/glm)
endif(GLM_FOUND)

if(MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")	
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
endif(MSVC)

add_custom_command(TARGET VulkanApp POST_BUILD        # Adds a post-build event to VulkanApp
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${PROJECT_BINARY_DIR}/third-party/glfw-3.2.1/src/glfw3.dll"      # <--this is in-file
        "${PROJECT_BINARY_DIR}")										# <--this is out-file path

# copy files over

add_custom_command(TARGET VulkanApp POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:VulkanApp>/assets)
