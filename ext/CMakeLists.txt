
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_MultipleHeaders ON CACHE INTERNAL "")

find_path(JSON_PATH_FOUND "json" "")
find_path(FMT_PATH_FOUND "fmt" "")
find_path(SHADERC_PATH_FOUND "shaderc" "")

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