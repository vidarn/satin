cmake_minimum_required(VERSION 2.6)

set(VLIB_SRC PARENT_SCOPE
	${VLIB_INC_DIR}/sort/sort.c
	${VLIB_INC_DIR}/memory/buffer.c
	${VLIB_INC_DIR}/linalg/linalg.c
)
project(satin)
include_directories(${VLIB_INC_DIR})

set(SATIN_SRC_DIR PARENT_SCOPE ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(WIN32)
	set(SATIN_OS_SRC
		${SATIN_SRC_DIR}/os/win32/os.c
		${SATIN_SRC_DIR}/opengl/win32/opengl.c
	)
	set(SATIN_OS_LIBS
		Xinput.lib 
	)
endif(WIN32)
if(UNIX)
    if(APPLE)
        find_package(GLUT REQUIRED)
        set(SATIN_OS_SRC
            ${SATIN_SRC_DIR}/os/osx/os.m
            ${SATIN_SRC_DIR}/opengl/osx/opengl.c
        )
        set(SATIN_OS_LIBS
            ${GLUT_glut_LIBRARY}
        )
        include_directories(${GLUT_INCLUDE_DIR} )
    elseif(EMSCRIPTEN)
        set(SATIN_OS_SRC
            ${SATIN_SRC_DIR}/os/emscripten/os.c
            ${SATIN_SRC_DIR}/opengl/emscripten/opengl.c
        )
        set(SATIN_OS_LIBS
        )
    else()
        set(SATIN_OS_SRC
            ${SATIN_SRC_DIR}/os/gnu/os.c
            ${SATIN_SRC_DIR}/opengl/gnu/opengl.c
        )
        set(SATIN_OS_LIBS
            m
            X11
        )
    endif(APPLE)
endif(UNIX)
set(SATIN_SRC PARENT_SCOPE
	${SATIN_SRC_DIR}/engine.c
	${SATIN_SRC_DIR}/gui/gui.c
	${SATIN_OS_SRC}
)
include_directories(${SATIN_SRC_DIR})
include_directories(${SATIN_SRC_DIR}/opengl)

#add_library(satin STATIC ${SATIN_SRC} ${VLIB_SRC})

if(APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
endif(APPLE)


#target_compile_options(satin PRIVATE /arch:AVX2)

#find_package(GLEW REQUIRED)
find_package(OpenGL 3.2 REQUIRED)
#include_directories(${GLEW_INCLUDE_DIRS})
#target_link_libraries(satin ${OPENGL_LIBRARIES} ${SATIN_OS_LIBS})
#target_link_libraries(satin Xinput.lib ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})

