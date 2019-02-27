#pragma once
#include <GL/gl.h>
#ifdef WIN32
#include <Windows.h>
#include "wgl.h"
#include "glext.h"
#endif

#define OPENGL_FUNC(type, name) extern type name;
#include "opengl_funcs.h"
#ifdef WIN32
OPENGL_FUNC(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB)
OPENGL_FUNC(PFNGLACTIVETEXTUREPROC, glActiveTexture)
#endif
#undef OPENGL_FUNC

void opengl_load(void)
;

