target_sources(
    VulkanEngine
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Water.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/PlayerController.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Scene.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Skybox.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Terrain.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/TerrainSystem.cpp)
