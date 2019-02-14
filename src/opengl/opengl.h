#pragma once
#include <Windows.h>
#include <gl\GL.h>
#include "wgl.h"
#include "glext.h"

#define OPENGL_FUNC(type, name) extern type name;
#include "opengl_funcs.h"
#undef OPENGL_FUNC

void opengl_load(void)
;
