cmake_minimum_required (VERSION 3.8)

project (SpatialLib)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wfloat-equal -Wundef -Wcast-align -Wwrite-strings -Wmissing-declarations -Wredundant-decls -Wshadow -Woverloaded-virtual -Wimplicit-fallthrough -Wsign-conversion -Rpass=inline -Winline -std=c++${CMAKE_CXX_STANDARD} -Ofast")
set (CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE
	source_files
	${PROJECT_SOURCE_DIR}/*.hpp
)

file(GLOB_RECURSE
	test_files
	${PROJECT_SOURCE_DIR}/tests/**.cpp
	${PROJECT_SOURCE_DIR}/tests/**.hpp
)

add_executable (SpatialLibTest ${source_files} ${test_files})

set_target_properties(SpatialLibTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/")
target_compile_options (SpatialLibTest PUBLIC -fexceptions)

file(GLOB_RECURSE
	expirement_files
	${PROJECT_SOURCE_DIR}/expirements/**.cpp
	${PROJECT_SOURCE_DIR}/expirements/**.hpp
)

add_executable (KDTreeExpirements ${source_files} ${expirement_files})

set_target_properties(KDTreeExpirements PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/")
target_compile_options (KDTreeExpirements PUBLIC -fexceptions)