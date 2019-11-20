cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

include(CMakeToolsHelpers OPTIONAL)

set(CMAKE_CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(VulkanRenderer VERSION 0.0.1)

#libraries
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Vulkan REQUIRED)

add_subdirectory(ext)

set(GLSLANG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/glslang)
set(SPIRV_CROSS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/spirv-cross)
set(ENTT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/entt)

#gtk include for linux file window
if(UNIX)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

	include_directories(${GTK3_INCLUDE_DIRS})
	link_directories(${GTK3_LIBRARY_DIRS})
	add_definitions(${GTK3_CFLAGS_OTHER}) 
endif(UNIX)

# main files to compile
add_library(VulkanEngine STATIC)
add_subdirectory(engine)

target_include_directories(VulkanEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine)
target_include_directories(VulkanEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine/third-party)

if(MSVC)
	target_compile_options(VulkanEngine PRIVATE /std:c++17 /permissive-)
endif(MSVC)

OPTION(TSAN "Enable thread sanitizer on unix platforms" OFF)
OPTION(ASAN "Enable address sanitizer on unix platforms" ON)
OPTION(UBSAN "Enable undefined behavior sanitizer on unix platforms" OFF)

if(UNIX)
	target_compile_options(VulkanEngine PUBLIC -pthread)
	
	#g++-8 still requires this, not necessary with g++-9
	target_link_libraries(VulkanEngine PRIVATE stdc++fs)
	
	if(DEBUG)
		target_compile_options(VulkanEngine PUBLIC -Wextra -Wall)
		target_link_options(VulkanEngine PUBLIC -fno-omit-frame-pointer)
	endif(DEBUG)
	if(TSAN)
		target_compile_options(VulkanEngine PUBLIC -fsanitize=thread)
		target_link_options(VulkanEngine PUBLIC -fsanitize=thread)
	endif(TSAN)
	if(ASAN)
		target_compile_options(VulkanEngine PUBLIC -fsanitize=address)
		target_link_options(VulkanEngine PUBLIC -fsanitize=address)
	endif(ASAN)
	if(UBSAN)
		target_compile_options(VulkanEngine PUBLIC -fsanitize=undefined)
	endif(UBSAN)
endif(UNIX)

#cml
target_link_libraries(VulkanEngine PUBLIC cml)

#json
target_link_libraries(VulkanEngine PUBLIC nlohmann_json)

#fmt
target_link_libraries(VulkanEngine PUBLIC fmt)

#SG14
target_include_directories(VulkanEngine PUBLIC ${CMAKE_SOURCE_DIR}/ext/SG14)

#glfw
target_link_libraries(VulkanEngine PUBLIC glfw)

#vulkan
target_include_directories(VulkanEngine PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(VulkanEngine PRIVATE ${Vulkan_LIBRARIES})

#glslang
target_include_directories(VulkanEngine PUBLIC ${GLSLANG_INCLUDE_DIR})
target_link_libraries(VulkanEngine PUBLIC glslang SPIRV)

#spirv-cross
target_include_directories(VulkanEngine PUBLIC ${SPIRV_CROSS_INCLUDE_DIR})
target_link_libraries(VulkanEngine PUBLIC spirv-cross-core spirv-cross-cpp spirv-cross-reflect) 

#entt
target_link_libraries(VulkanEngine PUBLIC EnTT)
target_include_directories(VulkanEngine PUBLIC ${ENTT_INCLUDE_DIR}/src)

#fx-gltf
target_link_libraries(VulkanEngine PUBLIC fx-gltf)

#imgui
target_link_libraries(VulkanEngine PUBLIC ocornut::imgui)
target_include_directories(VulkanEngine PUBLIC ext/imgui)

#fast-noise-simd
target_link_libraries(VulkanEngine PUBLIC fast-noise-simd)
target_include_directories(VulkanEngine PUBLIC ext/FastNoiseSIMD)

#stb
target_link_libraries(VulkanEngine PUBLIC stb)

#vma
target_link_libraries(VulkanEngine PUBLIC VulkanMemoryAllocator)

#gli
target_include_directories(VulkanEngine PUBLIC ext/gli)
target_include_directories(VulkanEngine PUBLIC ext/gli/external)


#gtk
if(UNIX)
	target_link_libraries(VulkanEngine PRIVATE ${GTK3_LIBRARIES})
endif(UNIX)

# copy files over
add_custom_target(copy_assets ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory 
	${PROJECT_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
	DEPENDS VulkanEngine
	COMMENT "copying assets to build directory")

add_dependencies(copy_assets VulkanEngine)

add_executable(VulkanEditor)
add_subdirectory(editor)

target_link_libraries(VulkanEditor PUBLIC VulkanEngine)
target_include_directories(VulkanEditor PRIVATE ${PROJECT_SOURCE_DIR}/engine)
