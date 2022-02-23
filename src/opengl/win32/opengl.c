#include "../opengl.h"
#define OPENGL_FUNC(type, name) type name;
#include "../opengl_funcs.h"
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

struct OpenGLFuncPointers {
	#define OPENGL_FUNC(type, name) type name;
	#include "../opengl_funcs.h"
	#undef OPENGL_FUNC
};

void opengl_activate(struct OpenGLFuncPointers *ptrs) {
	#define OPENGL_FUNC(type, name) name = ptrs->name;
	#include "../opengl_funcs.h"
    #undef OPENGL_FUNC
}

struct OpenGLFuncPointers *opengl_load(void)
{
	HMODULE opengl32_dll = LoadLibraryA("opengl32.dll");
	struct OpenGLFuncPointers* ptrs = calloc(1, sizeof(struct OpenGLFuncPointers));

	#define OPENGL_FUNC(type, name) ptrs->name = get_gl_func_pointer(#name, opengl32_dll);
	#include "../opengl_funcs.h"
    #undef OPENGL_FUNC

    opengl_activate(ptrs);
	return ptrs;
}