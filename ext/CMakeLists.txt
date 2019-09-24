
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

add_subdirectory(SG14)

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

target_compile_options(fast-noise-simd PRIVATE -march=core-avx2 -msse2 -msse4.1 -Wimplicit-fallthrough=0)


if(NOT STB_PATH_FOUND)
	message("Updating stb libraries")
	execute_process(COMMAND git submodule update --init --recursive stb 
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT STB_PATH_FOUND)

add_library(stb INTERFACE)

target_include_directories(stb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/stb/)