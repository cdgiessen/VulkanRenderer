# Find Vulkan
#
# VULKAN_INCLUDE_DIR
# VULKAN_LIBRARY
# VULKAN_FOUND

if (WIN32)
    find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS
        "$ENV{VULKAN_SDK}/Include"
        "$ENV{VK_SDK_PATH}/Include")
    if (CMAKE_CL_64)
        find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS
            "$ENV{VULKAN_SDK}/Bin"
			"$ENV{VULKAN_SDK}/Lib"
            "$ENV{VK_SDK_PATH}/Bin"
			"$ENV{VK_SDK_PATH}/Lib")
        find_library(VULKAN_STATIC_LIBRARY NAMES vkstatic.1 HINTS
            "$ENV{VULKAN_SDK}/Bin"
			"$ENV{VULKAN_SDK}/Lib"
            "$ENV{VK_SDK_PATH}/Bin"
			"$ENV{VK_SDK_PATH}/Lib")
    else()
        find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS
            "$ENV{VULKAN_SDK}/Bin32"
			"$ENV{VULKAN_SDK}/Lib32"
            "$ENV{VK_SDK_PATH}/Bin32"
			"$ENV{VK_SDK_PATH}/Lib32")
    endif()
else()
    find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS
        "$ENV{VULKAN_SDK}/include")
    find_library(VULKAN_LIBRARY NAMES vulkan HINTS
        "$ENV{VULKAN_SDK}/lib")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vulkan DEFAULT_MSG VULKAN_LIBRARY VULKAN_INCLUDE_DIR)

mark_as_advanced(VULKAN_INCLUDE_DIR VULKAN_LIBRARY VULKAN_STATIC_LIBRARY)

