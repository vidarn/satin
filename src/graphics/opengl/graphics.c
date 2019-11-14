#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "graphics/graphics.h"

#define SATIN_SHADER_SUFFIX ""
#ifdef _WIN32
	#include "opengl/opengl.h"
#endif
#ifdef __APPLE__
    #include "TargetConditionals.h"
	#include "opengl/opengl.h"
#endif

static const char *glsl_version_string = "#version 330\n";

struct GraphicsData
{
    float clear_color[4];
};

struct Shader
{
    int handle;
};

struct Mesh
{
    int num_vertex_buffers;
    GLuint vertex_array;
    GLuint vertex_buffers[16];
    GLuint index_buffer;
    int num_indices;
};

struct Texture
{
    int dummy;
};

int graphics_value_sizes[] = {
#define GRAPHICS_VALUE_TYPE(name,num,size) num*size,
    GRAPHICS_VALUE_TYPES
#undef GRAPHICS_VALUE_TYPE
};

int graphics_value_nums[] = {
#define GRAPHICS_VALUE_TYPE(name,num,size) num,
    GRAPHICS_VALUE_TYPES
#undef GRAPHICS_VALUE_TYPE
};

int graphics_value_opengl_types[] = {
    [GRAPHICS_VALUE_INT] = GL_INT,
    [GRAPHICS_VALUE_FLOAT] = GL_FLOAT,
    [GRAPHICS_VALUE_MAT4] = GL_FLOAT,
    [GRAPHICS_VALUE_MAT3] = GL_FLOAT,
    [GRAPHICS_VALUE_VEC4] = GL_FLOAT,
    [GRAPHICS_VALUE_VEC3] = GL_FLOAT,
    [GRAPHICS_VALUE_VEC2] = GL_FLOAT,
    [GRAPHICS_VALUE_HALF] = GL_HALF_FLOAT,
    [GRAPHICS_VALUE_HALF2] = GL_HALF_FLOAT,
    [GRAPHICS_VALUE_HALF3] = GL_HALF_FLOAT,
    [GRAPHICS_VALUE_TEX2] = GL_INT,
};

struct GraphicsData *graphics_create(void * param)
{
    struct GraphicsData *graphics = calloc(1,sizeof(struct GraphicsData));
    return graphics;
}

void graphics_begin_render_pass(float *clear_rgba, struct GraphicsData *graphics)
{
    memcpy(graphics->clear_color, clear_rgba, 4*4);
}

void graphics_end_render_pass(struct GraphicsData *graphics)
{
}

void graphics_clear(struct GraphicsData *graphics)
{
    glClearColor(
            graphics->clear_color[0],
            graphics->clear_color[1],
            graphics->clear_color[2],
            graphics->clear_color[3]
            );
    glClear(GL_COLOR_BUFFER_BIT);
}

void graphics_render_mesh(struct Mesh *mesh, struct Shader *shader, struct GraphicsValueSpec *uniform_specs, int num_uniform_specs, struct GraphicsData *graphics)
{
}

struct Shader *graphics_compile_shader(const char *vert_source, const char *frag_source,
    enum GraphicsBlendMode blend_mode, char *error_buffer, int error_buffer_len, struct GraphicsData *graphics)
{
    const char *version_string = "#version 330\n";
    struct Shader *shader = calloc(1,sizeof(struct Shader));
    
	int shader_handle=glCreateProgram();
	int vert_handle=glCreateShader(GL_VERTEX_SHADER);
	int frag_handle=glCreateShader(GL_FRAGMENT_SHADER);
    const char *vert_char[2] = {version_string,vert_source};
    const char *frag_char[2] = {version_string,frag_source};
	glShaderSource(vert_handle,2,vert_char,0);
	glShaderSource(frag_handle,2,frag_char,0);
	glCompileShader(vert_handle);
	if(check_compilation(vert_handle,GL_COMPILE_STATUS,
		error_buffer,error_buffer_len,"vertex shader")!=GL_TRUE)
	{
		return -1;
	}
	glCompileShader(frag_handle);
	if(check_compilation(frag_handle,GL_COMPILE_STATUS,
		error_buffer,error_buffer_len,"fragment shader")!=GL_TRUE)
	{
		return -1;
	}
	glAttachShader(shader_handle,vert_handle);
	glAttachShader(shader_handle,frag_handle);
	glLinkProgram(shader_handle);
	if(check_compilation(shader_handle,GL_LINK_STATUS,
		error_buffer,error_buffer_len,"shader program")!=GL_TRUE)
	{
		return -1;
	}

    shader->handle = shader_handle;

    return shader;
}

struct Mesh *graphics_create_mesh(struct GraphicsValueSpec *value_specs, uint32_t num_value_specs, uint32_t num_verts, int *index_data, uint32_t num_indices, struct GraphicsData *graphics)
{
    struct Mesh *mesh = calloc(1,sizeof(struct Mesh));

    mesh->num_vertex_buffers = num_value_specs;
    glGenVertexArrays(1,&mesh->vertex_array);
    glBindVertexArray(mesh->vertex_array);
    assert(num_value_specs < 16);
    glGenBuffers(num_value_specs,mesh->vertex_buffers);
    for(int i=0;i< num_value_specs; i++){
        struct GraphicsValueSpec spec = value_specs[i];
        glBindBuffer(GL_ARRAY_BUFFER,mesh->vertex_buffers[i]);
    
        glBufferData(GL_ARRAY_BUFFER,graphics_value_sizes[spec.type]*num_verts
            ,spec.data, GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i,graphics_value_nums[spec.type],
            graphics_value_opengl_types[spec.type],GL_FALSE,
			0, 0);
    }
    mesh->num_indices = num_indices;

    glGenBuffers(1,&mesh->index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh->index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,num_indices*4,index_data,
                 GL_STATIC_DRAW);

    return mesh;
}

struct Texture *graphics_create_texture(uint8_t *texture_data, uint32_t w, uint32_t h, uint32_t format, struct GraphicsData *graphics)
{
    struct Texture *texture = calloc(1,sizeof(struct Texture));
    return texture;
}

void graphics_set_depth_test(int enabled, struct GraphicsData *graphics)
{
}

