cmake_minimum_required(VERSION 2.6)
set(VLIB_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/vlib CACHE PATH "vlib include directory")
set(VLIB_SRC ${VLIB_INC_DIR}/sort/sort.c)
include_directories(${VLIB_INC_DIR})

project(satin)

set(SATIN_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SATIN_SRC
	${SATIN_SRC_DIR}/engine.c
	${SATIN_SRC_DIR}/os/win32/os.c
)
include_directories(${SATIN_SRC_DIR})

add_library(satin ${SATIN_SRC} ${VLIB_SRC})

find_package(GLEW REQUIRED)
find_package(OpenGL 3.2 REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
target_link_libraries(satin Xinput.lib ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})

