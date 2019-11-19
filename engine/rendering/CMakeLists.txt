target_sources(VulkanEngine PUBLIC

${CMAKE_CURRENT_SOURCE_DIR}/DoubleBuffer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/FrameGraph.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Lighting.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Renderer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/ViewCamera.cpp

${CMAKE_CURRENT_SOURCE_DIR}/DoubleBuffer.h
${CMAKE_CURRENT_SOURCE_DIR}/FrameGraph.h
${CMAKE_CURRENT_SOURCE_DIR}/Renderer.h
${CMAKE_CURRENT_SOURCE_DIR}/ViewCamera.h

)

add_subdirectory(backend)
add_subdirectory(renderers)