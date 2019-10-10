find_path(JSON_PATH_FOUND "json" "")
find_path(FMT_PATH_FOUND "fmt" "")
find_path(SG14_PATH_FOUND "SG14" "")
find_path(GLSLANG_PATH_FOUND "glslang" "")
find_path(SPIRV_CROSS_PATH_FOUND "spirv-cross" "")
find_path(CML_PATH_FOUND "cml" "")
find_path(FX_GLTF_PATH_FOUND "fx-gltf" "")
find_path(IMGUI_PATH_FOUND "imgui/imgui" "")
find_path(FASTNOISESIMD_PATH_FOUND "FastNoiseSIMD" "")
find_path(STB_PATH_FOUND "stb/stb" "")
find_path(VMA_PATH_FOUND "vma" "")
find_path(GLFW_PATH_FOUND "glfw" "")
find_path(GLI_PATH_FOUND "gli" "")


set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_MultipleHeaders ON CACHE INTERNAL "")

if(NOT JSON_PATH_FOUND)
	message("Updating nlohmann-json")
	execute_process(COMMAND git submodule update --init --recursive json
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT JSON_PATH_FOUND)

add_subdirectory(json)

if(NOT FMT_PATH_FOUND)
	message("Updating fmt library")
	execute_process(COMMAND git submodule update --init --recursive fmt
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT FMT_PATH_FOUND)

add_subdirectory(fmt)

if(NOT SG14_PATH_FOUND)
	message("Updating SG14 libraries")
	execute_process(COMMAND git submodule update --init --recursive SG14
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT SG14_PATH_FOUND)

add_library(sg14 INTERFACE)
target_include_directories(sg14 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/SG14)


set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT ON CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(NOT GLSLANG_PATH_FOUND)
	message("Updating glslang")
	execute_process(COMMAND git submodule update --init --recursive glslang
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT GLSLANG_PATH_FOUND)

add_subdirectory(glslang)

set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)


if(NOT SPIRV_CROSS_PATH_FOUND)
	message("Updating SPIRV-Cross")
	execute_process(COMMAND git submodule update --init --recursive spirv-cross
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT SPIRV_CROSS_PATH_FOUND)

add_subdirectory(spirv-cross)

if(NOT CML_PATH_FOUND)
	message("Updating cml")
	execute_process(COMMAND git submodule update --init --recursive cml
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT CML_PATH_FOUND)


add_subdirectory(cml)

set (FX_GLTF_INSTALL OFF CACHE BOOL "" FORCE)
set (BUILD_TESTING OFF CACHE BOOL "" FORCE)


if(NOT FX_GLTF_PATH_FOUND)
	message("Updating fx-gltf")
	execute_process(COMMAND git submodule update --init --recursive fx-gltf 
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT FX_GLTF_PATH_FOUND)

add_subdirectory(fx-gltf)

if(NOT IMGUI_PATH_FOUND)
	message("Updating imgui")
	execute_process(COMMAND git submodule update --init --recursive imgui/imgui 
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT IMGUI_PATH_FOUND)

add_library(dear-imgui OBJECT
	${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui/imgui.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui/imgui_draw.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui/imgui_widgets.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.hpp)

target_include_directories(dear-imgui PUBLIC imgui/imgui)

add_library(ocornut::imgui ALIAS dear-imgui)

if(NOT FASTNOISESIMD_PATH_FOUND)
	message("Updating FastNoiseSIMD")
	execute_process(COMMAND git submodule update --init --recursive FastNoiseSIMD 
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT FASTNOISESIMD_PATH_FOUND)

add_library(fast-noise-simd STATIC 
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD.h
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_internal.h
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_internal.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_avx2.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_avx512.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_neon.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_sse2.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD/FastNoiseSIMD_sse41.cpp
)

target_include_directories(fast-noise-simd PUBLIC FastNoiseSIMD)

if(UNIX)
target_compile_options(fast-noise-simd PRIVATE -march=core-avx2 -msse2 -msse4.1 -Wimplicit-fallthrough=0)
elseif(WIN32)
target_compile_options(fast-noise-simd PRIVATE /arch:AVX2)

endif()


if(NOT STB_PATH_FOUND)
	message("Updating stb libraries")
	execute_process(COMMAND git submodule update --init --recursive stb 
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT STB_PATH_FOUND)

add_library(stb INTERFACE)

target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb/)

if(NOT VMA_PATH_FOUND)
message("Updating VulkanMemoryAllocator")
execute_process(COMMAND git submodule update --init --recursive vma 
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT VMA_PATH_FOUND)

add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vma/src)

target_include_directories(VulkanMemoryAllocator INTERFACE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(VulkanMemoryAllocator INTERFACE ${Vulkan_LIBRARIES})

set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if(NOT GLFW_PATH_FOUND)
	message("Updating glfw")
	execute_process(COMMAND git submodule update --init --recursive glfw
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT GLFW_PATH_FOUND)

add_subdirectory(glfw)

if(NOT GLI_PATH_FOUND)
	message("Updating gli")
	execute_process(COMMAND git submodule update --init --recursive gli
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT GLI_PATH_FOUND)

