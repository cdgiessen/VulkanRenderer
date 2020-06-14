
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
option(GLFW_BUILD_EXAMPLES "" OFF)

option(BUILD_TESTING "" OFF)
option(JSON_Install "" OFF)
option(JSON_BuildTests "" OFF)
option(JSON_MultipleHeaders "" ON)

option(SKIP_GLSLANG_INSTALL "" ON)
option(ENABLE_HLSL "" OFF)
option(ENABLE_OPT "" ON)
option(ENABLE_GLSLANG_INSTALL "" OFF)
option(BUILD_SHARED_LIBS "" OFF)

option(SPIRV_CROSS_CLI "" OFF)
option(SPIRV_CROSS_ENABLE_MSL "" OFF)
option(SPIRV_CROSS_ENABLE_HLSL "" OFF)
option(SPIRV_CROSS_ENABLE_TESTS "" OFF)

include(FetchContent)
FetchContent_Declare(
    FastNoiseSIMD
    GIT_REPOSITORY https://github.com/Auburns/FastNoiseSIMD
    GIT_TAG        2b70661817f212f8463cb985e503797183889d34
)
FetchContent_Declare(
    SG14
    GIT_REPOSITORY https://github.com/WG21-SG14/SG14
    GIT_TAG        0e3fe16c8721a4fd7bcf169b029aa3864a3d047b
)
FetchContent_Declare(
    cml
    GIT_REPOSITORY https://github.com/cdgiessen/cml
    GIT_TAG        9945735cbd08ff9ba2bbe55f3c4b7556ae0827ba
)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        6.2.1
)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG        3.3.2
)
FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang
    GIT_TAG        8.13.3743
)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG        v1.76
)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG        v3.7.3
)
FetchContent_Declare(
    spirv_cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross
    GIT_TAG        2020-05-19
)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb
    GIT_TAG        f54acd4e13430c5122cab4ca657705c84aa61b08
)
FetchContent_Declare(
    vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG        511cac8db4f9d9d5eda95a6e16af74e0731f5f32
)
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG        v2.3.0
)

FetchContent_Declare(
    openxr
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK
    GIT_TAG        release-1.0.9
)

FetchContent_MakeAvailable(cml fmt glfw glslang json spirv_cross vk_bootstrap openxr)

FetchContent_GetProperties(SG14)
if(NOT SG14_POPULATED)
    FetchContent_Populate(SG14)

    add_library(SG14 INTERFACE)
    target_include_directories(SG14 INTERFACE ${sg14_SOURCE_DIR})
endif()

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)

    add_library(dear-imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.h 
	    ${imgui_SOURCE_DIR}/imgui.cpp 
	    ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.hpp)
    target_include_directories(dear-imgui PUBLIC ${imgui_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
endif()

FetchContent_GetProperties(FastNoiseSIMD)
if(NOT FastNoiseSIMD_POPULATED)
    FetchContent_Populate(FastNoiseSIMD)
    add_library(fast_noise_simd STATIC 
        ${fastnoisesimd_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD.h
        ${fastnoisesimd_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD.cpp
        ${fastnoisesimd_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_internal.h
        ${fastnoisesimd_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_internal.cpp
        ${fastnoisesimd_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_avx2.cpp
        ${fastnoisesimd_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_avx512.cpp
        ${fastnoisesimd_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_neon.cpp
        ${fastnoisesimd_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_sse2.cpp
        ${fastnoisesimd_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_sse41.cpp
    )

    target_include_directories(fast_noise_simd PUBLIC ${fastnoisesimd_SOURCE_DIR})

    if(UNIX)
        target_compile_options(fast_noise_simd PRIVATE -march=core-avx2 -msse2 -msse4.1 -Wimplicit-fallthrough=0)
    elseif(WIN32)
        target_compile_options(fast_noise_simd PRIVATE /arch:AVX2)
    endif()
endif()

FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
    add_library(stb INTERFACE)

    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()

FetchContent_GetProperties(vma)
if(NOT vma_POPULATED)
    FetchContent_Populate(vma)
    add_library(VulkanMemoryAllocator INTERFACE)
    target_include_directories(VulkanMemoryAllocator INTERFACE ${vma_SOURCE_DIR}/src)

    target_include_directories(VulkanMemoryAllocator INTERFACE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(VulkanMemoryAllocator INTERFACE ${Vulkan_LIBRARIES})
endif()

add_library(openxr INTERFACE)
target_include_directories(openxr INTERFACE ${openxr_SOURCE_DIR}/include)