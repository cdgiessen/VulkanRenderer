target_sources(VulkanApp PUBLIC

${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imgui.h
${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imgui.cpp
${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imgui_draw.cpp

${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD.h
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_avx2.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_avx512.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_internal.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_neon.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_sse2.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FastNoiseSIMD/FastNoiseSIMD_sse41.cpp

${CMAKE_CURRENT_SOURCE_DIR}/stb_image/stb_image.h
${CMAKE_CURRENT_SOURCE_DIR}/stb_image/stb_image_write.h
${CMAKE_CURRENT_SOURCE_DIR}/stb_image/stb_image_resize.h

${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/vk_mem_alloc.h
)