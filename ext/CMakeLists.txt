
find_path(JSON_PATH_FOUND "json" "")
find_path(FMT_PATH_FOUND "fmt" "")
find_path(SG14_PATH_FOUND "SG14" "")
find_path(GLSLANG_PATH_FOUND "glslang" "")
find_path(SPIRV_CROSS_PATH_FOUND "spirv-cross" "")
find_path(CML_PATH_FOUND "cml" "")
find_path(DTEX_PATH_FOUND "DTex" "")

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_MultipleHeaders ON CACHE INTERNAL "")

if(NOT JSON_PATH_FOUND)
	message("Updating nlohmann-json")
	execute_process(COMMAND git submodule update --init --recursive json
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT JSON_PATH_FOUND)

add_subdirectory(json)

if(NOT FMT_PATH_FOUND)
	message("Updating fmtlib")
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

if(NOT DTEX_PATH_FOUND)
	message("Updating DTex")
	execute_process(COMMAND git submodule update --init --recursive DTex 
					WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT DTEX_PATH_FOUND)

add_subdirectory(DTex)