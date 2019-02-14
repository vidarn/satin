cmake_minimum_required(VERSION 2.6)

set(VLIB_SRC
	${VLIB_INC_DIR}/sort/sort.c
)
project(satin)
include_directories(${VLIB_INC_DIR})

set(SATIN_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SATIN_SRC
	${SATIN_SRC_DIR}/engine.c
	${SATIN_SRC_DIR}/os/win32/os.c
	${SATIN_SRC_DIR}/opengl/win32/opengl.c
)
include_directories(${SATIN_SRC_DIR})
include_directories(${SATIN_SRC_DIR}/opengl)

add_library(satin ${SATIN_SRC} ${VLIB_SRC})

target_compile_options(satin PRIVATE /arch:AVX2)

#find_package(GLEW REQUIRED)
find_package(OpenGL 3.2 REQUIRED)
#include_directories(${GLEW_INCLUDE_DIRS})
target_link_libraries(satin Xinput.lib ${OPENGL_LIBRARIES})
#target_link_libraries(satin Xinput.lib ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})

