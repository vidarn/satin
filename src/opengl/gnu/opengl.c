#include "opengl.h"
#include <GL/glx.h>
#define OPENGL_FUNC(type, name) type name;
#include "opengl_funcs.h"
#undef OPENGL_FUNC

void opengl_load(void)
{
	#define OPENGL_FUNC(type, name) name = (void*)glXGetProcAddress(#name);
	#include "opengl_funcs.h"
	#undef OPENGL_FUNC

}
