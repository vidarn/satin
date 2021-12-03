#include "../opengl.h"
#define OPENGL_FUNC(type, name) type name;
#include "../opengl_funcs.h"
OPENGL_FUNC(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB)
OPENGL_FUNC(PFNGLACTIVETEXTUREPROC, glActiveTexture)
#undef OPENGL_FUNC

static void *get_gl_func_pointer(const char *name, HMODULE opengl32_dll)
{
	void *p = (void *)GetProcAddress(opengl32_dll, name);
	if (!p)
	{
		p = (void *)wglGetProcAddress(name);
	}

	return p;
}

void opengl_load(void)
{
	HMODULE opengl32_dll = LoadLibraryA("opengl32.dll");

	#define OPENGL_FUNC(type, name) name = get_gl_func_pointer(#name, opengl32_dll);
	#include "../opengl_funcs.h"
	OPENGL_FUNC(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB)
	OPENGL_FUNC(PFNGLACTIVETEXTUREPROC, glActiveTexture)
	#undef OPENGL_FUNC

}
