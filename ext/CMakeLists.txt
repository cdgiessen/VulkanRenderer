
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_MultipleHeaders ON CACHE INTERNAL "")

find_path(JSON_PATH_FOUND "json" "")
find_path(SPIRV_CROSS_PATH_FOUND "SPIRV-Cross" "")
find_path(GLSLANG_PATH_FOUND "glslang" "")
find_path(FMT_PATH_FOUND "fmt" "")

if(NOT JSON_PATH_FOUND)
	message("Updating nlohmann-json")
	execute_process(COMMAND git submodule update --init --recursive json
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT JSON_PATH_FOUND)

add_subdirectory(json)

if(NOT SPIRV_CROSS_PATH_FOUND)
	message("Updating spirv-cross")
	execute_process(COMMAND git submodule update --init --recursive SPIRV-Cross
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT SPIRV_CROSS_PATH_FOUND)

add_subdirectory(SPIRV-Cross)

set(ENABLE_HLSL OFF CACHE INTERNAL "")
set(ENABLE_NV_EXTENSIONS OFF CACHE INTERNAL "")
set(ENABLE_GLSLANG_BINARIES OFF CACHE INTERNAL "")



if(NOT GLSLANG_PATH_FOUND)
	message("Updating glslang")
	execute_process(COMMAND git submodule update --init --recursive glslang
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT GLSLANG_PATH_FOUND)

add_subdirectory(glslang)

if(NOT FMT_PATH_FOUND)
	message("Updating fmtlib")
	execute_process(COMMAND git submodule update --init --recursive fmt
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT FMT_PATH_FOUND)

add_subdirectory(fmt)
