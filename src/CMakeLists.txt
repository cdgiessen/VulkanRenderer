add_subdirectory(core)
add_subdirectory(gui)
add_subdirectory(rendering)
add_subdirectory(resources)
add_subdirectory(scene)
add_subdirectory(util)

target_include_directories(VulkanApp PRIVATE third-party)
