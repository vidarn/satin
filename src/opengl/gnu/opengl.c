#include "opengl.h"
#define GLX_MESA_swap_control 1
#include <GL/glx.h>
#define OPENGL_FUNC(type, name) type name;
#include "opengl_funcs.h"
#undef OPENGL_FUNC
typedef int (* PFNGLXSWAPINTERVALMESAPROC) (unsigned int interval);
PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESA;

void opengl_load(void)
{
	#define OPENGL_FUNC(type, name) name = (void*)glXGetProcAddress(#name);
	#include "opengl_funcs.h"
	#undef OPENGL_FUNC
	glXSwapIntervalMESA = (void*)glXGetProcAddress("glXSwapIntervalMESA");

}
