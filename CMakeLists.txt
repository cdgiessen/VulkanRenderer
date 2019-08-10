cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

include(CMakeToolsHelpers OPTIONAL)

set(CMAKE_CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(VulkanRenderer VERSION 0.0.1)

#libraries
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

add_subdirectory(ext)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Bullet REQUIRED) 
find_package(OpenAL REQUIRED)

set(GLSLANG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/glslang)
set(SPIRV_CROSS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/spirv-cross)
set(ENTT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/entt)

#gtk include for linux file window
if(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")

	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

	include_directories(${GTK3_INCLUDE_DIRS})
	link_directories(${GTK3_LIBRARY_DIRS})
	add_definitions(${GTK3_CFLAGS_OTHER}) 
endif(UNIX)

# main files to compile
add_library(VulkanApp)
add_subdirectory(src)

#include the different source directories in include path
target_include_directories(VulkanApp PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(VulkanApp PUBLIC ${CMAKE_SOURCE_DIR}/src/third-party)

if(MSVC)
	target_compile_options(VulkanApp PRIVATE "/std:c++17")
	target_compile_options(VulkanApp PRIVATE "/permissive-")
endif(MSVC)

OPTION(TSAN "Enable thread sanitizer on unix platforms" OFF)
OPTION(ASAN "Enable address sanitizer on unix platforms" ON)
OPTION(LSAN "Enable leak sanitizer on unix platforms" ON)
OPTION(UBSAN "Enable undefined behavior sanitizer on unix platforms" OFF)

if(UNIX)
	if(DEBUG)
		set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wextra -Wall")
		set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer")
	endif(DEBUG)
	if(TSAN)
		set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread")
	endif(TSAN)
	if(ASAN)
		set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
	endif(ASAN)
	if(LSAN)
		set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
		set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")

	endif(LSAN)
	if(UBSAN)
		set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined")
	endif(UBSAN)

	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif(UNIX)

#cml
target_link_libraries(VulkanApp PUBLIC cml)

#json
target_link_libraries(VulkanApp PUBLIC nlohmann_json)

#fmt
target_link_libraries(VulkanApp PUBLIC fmt)

#SG14
target_include_directories(VulkanApp PRIVATE ${SG14_INCLUDE_DIRECTORY})

#DTex
target_link_libraries(VulkanApp PUBLIC DTex::DTex)

#glfw
target_link_libraries(VulkanApp PRIVATE glfw)

#vulkan
target_include_directories(VulkanApp PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(VulkanApp PRIVATE ${Vulkan_LIBRARIES})

#glslang
target_include_directories(VulkanApp PUBLIC ${GLSLANG_INCLUDE_DIR})
target_link_libraries(VulkanApp PUBLIC glslang SPIRV)

#spirv-cross
target_include_directories(VulkanApp PUBLIC ${SPIRV_CROSS_INCLUDE_DIR})
target_link_libraries(VulkanApp PUBLIC spirv-cross-core spirv-cross-cpp spirv-cross-reflect) 

#bullet physics
target_include_directories(VulkanApp PRIVATE ${BULLET_INCLUDE_DIRS})
target_link_libraries(VulkanApp PRIVATE ${BULLET_LIBRARIES})

#openal
target_link_libraries(VulkanApp PRIVATE ${OPENAL_LIBRARIES})
target_include_directories(VulkanApp PRIVATE ${OPENAL_INCLUDE_DIRS})

#entt
target_include_directories(VulkanApp PUBLIC ${ENTT_INCLUDE_DIR}/src/)

#gtk
if(UNIX)
	target_link_libraries(VulkanApp PRIVATE ${GTK3_LIBRARIES})

	#g++-8 still requires this, not necessary with g++-9
	target_link_libraries(VulkanApp PRIVATE stdc++fs)

	target_compile_options(VulkanApp PRIVATE -Wall -Wextra)
endif(UNIX)

# copy files over
add_custom_command(TARGET VulkanApp POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:VulkanApp>/assets)

add_executable(run_VulkanApp ${PROJECT_SOURCE_DIR}/src/core/main.cpp)

target_link_libraries(run_VulkanApp PRIVATE VulkanApp)