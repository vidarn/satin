#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef EMSCRIPTEN
#include <GLES3/gl3.h>
#else

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else

#include <GL/gl.h>

#ifdef _WIN32
#include "wgl.h"
#include "glext.h"
#endif

#define OPENGL_FUNC(type, name) extern type name;
#include "opengl_funcs.h"
#undef OPENGL_FUNC
#endif

#endif

struct OpenGLFuncPointers
;
struct OpenGLFuncPointers* opengl_load(void)
;
void opengl_activate(struct OpenGLFuncPointers *ptrs)
;

