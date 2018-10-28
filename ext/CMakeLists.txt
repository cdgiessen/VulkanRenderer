
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_MultipleHeaders ON)
add_subdirectory(json)

add_subdirectory(SPIRV-Cross)

add_subdirectory(glslang)