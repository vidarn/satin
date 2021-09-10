#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "graphics/graphics.h"
#include "os/os.h"
#include "window/window.h"

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
	enum GraphicsBlendMode blend_mode;
	int shader_handle;
};

struct Shader
{
    char *vert_filename;
    char *frag_filename;
    int valid;
    int handle;
	enum GraphicsBlendMode blend_mode;
};

struct Mesh
{
    GLuint vertex_array;
    GLuint vertex_buffers[16];
    GLuint index_buffer;
    int num_indices, num_verts;
    int num_vertex_buffers;
};

struct Texture
{
    unsigned int handle;
	GLenum gl_format;
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

void graphics_set_viewport(int x, int y, int w, int h)
{
    glViewport(x, y, w, h);
}

void graphics_begin_render_pass(float *clear_rgba, struct GraphicsData *graphics)
{
    memcpy(graphics->clear_color, clear_rgba, 4*4);
	graphics->blend_mode = GRAPHICS_BLEND_MODE_NONE;
	graphics->shader_handle = -1;
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
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void graphics_render_mesh(struct Mesh *mesh, struct Shader *shader,
        struct GraphicsValueSpec *uniform_specs, int num_uniform_specs,
        struct GraphicsData *graphics)
{

	if (shader->blend_mode != graphics->blend_mode) {
		if(graphics->blend_mode == GRAPHICS_BLEND_MODE_NONE){
			glEnable(GL_BLEND);
		}
		switch(shader->blend_mode){
			case GRAPHICS_BLEND_MODE_PREMUL:
				glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
				break;
			case GRAPHICS_BLEND_MODE_MULTIPLY:
				glBlendFunc(GL_DST_COLOR,GL_ZERO);
				break;
			case GRAPHICS_BLEND_MODE_NONE:
				glDisable(GL_BLEND);
				break;
		}
		graphics->blend_mode = shader->blend_mode;
	}

    int shader_handle = shader->handle;
	if (shader_handle != graphics->shader_handle) {
		glUseProgram(shader_handle);
		graphics->shader_handle = shader_handle;
	}
    glBindVertexArray(mesh->vertex_array);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh->index_buffer);

    int num_textures = 0;
    for(int j=0;j<num_uniform_specs;j++){
        struct GraphicsValueSpec u = uniform_specs[j];
        int loc=glGetUniformLocation(shader_handle,u.name);
        if(loc == -1){
            printf("Warning: Could not find \"%s\" uniform in shader (%s,%s)\n",
                u.name, shader->vert_filename, shader->frag_filename);
        }else{
            switch(u.type){
                case GRAPHICS_VALUE_INT:
                    glUniform1iv(loc, u.num, u.data);
                    break;
                case GRAPHICS_VALUE_FLOAT:
                    glUniform1fv(loc, u.num, u.data);
                    break;
                case GRAPHICS_VALUE_MAT4:
                    glUniformMatrix4fv(loc,u.num,GL_FALSE,u.data);
                    break;
                case GRAPHICS_VALUE_MAT3:
                    glUniformMatrix3fv(loc,u.num,GL_FALSE,u.data);
                    break;
                case GRAPHICS_VALUE_VEC4:
                    glUniform4fv(loc,u.num,u.data);
                    break;
                case GRAPHICS_VALUE_VEC3:
                    glUniform3fv(loc,u.num,u.data);
                    break;
                case GRAPHICS_VALUE_VEC2:
                    glUniform2fv(loc,u.num,u.data);
                    break;
                case GRAPHICS_VALUE_TEX2:
                {
                    struct Texture *tex = u.data;
                    glUniform1i(loc, num_textures);
                    glActiveTexture(GL_TEXTURE0 + num_textures);
                    glBindTexture(GL_TEXTURE_2D, tex->handle);
                    num_textures++;
                    break;
                }
                default:
                    break;
            }
        }
    }

    glDrawElements(GL_TRIANGLES, mesh->num_indices,GL_UNSIGNED_INT,(void*)0);
}

static GLint check_compilation(GLuint handle,GLenum flag,
	char *error_buffer,size_t error_buffer_len,const char *name)
{
	GLint ret = GL_FALSE;
	if(flag==GL_COMPILE_STATUS){
		glGetShaderiv(handle,flag,&ret);
	}
	if(flag==GL_LINK_STATUS){
		glGetProgramiv(handle,flag,&ret);
	}
	if(ret!=GL_TRUE){
		sprintf(error_buffer,"%s:\n",name);
		size_t buff_len=strlen(error_buffer);
		GLsizei len = 0;
		if(flag==GL_COMPILE_STATUS){
			glGetShaderInfoLog(handle,(GLsizei)(error_buffer_len-buff_len),&len,
               error_buffer+buff_len);
		}
		if(flag==GL_LINK_STATUS){
			glGetProgramInfoLog(handle,(GLsizei)(error_buffer_len-buff_len),
                &len,error_buffer+buff_len);
		}
		error_buffer[len+buff_len]=0;
	}
	return ret;
}

static void print_shader_listing(const char *shader_source,
        const char *shader_filename, const char *shader_type)
{
    printf("  ~ %s shader \"%s\" ~\n", shader_type, shader_filename);
    int line_no = 0;
    printf("%02d| ", line_no++);
    const char *c = shader_source;
    while(*c != 0){
        putchar(*c);
        if(*c=='\n'){
            printf("%02d| ", line_no++);
        }
        c++;
    }
    printf("\n");
}

struct Shader *graphics_compile_shader(const char *vert_filename, const char *frag_filename,
    enum GraphicsBlendMode blend_mode, char *error_buffer, int error_buffer_len, struct GraphicsData *graphics,
    struct WindowData *window_data)
{
    const char *version_string = "#version 330\n";
    
    struct Shader *shader = calloc(1,sizeof(struct Shader));
    shader->vert_filename = strdup(vert_filename);
    shader->frag_filename = strdup(frag_filename);

    printf("Compile shader %s %s\n", vert_filename, frag_filename);
    FILE *vert_fp = open_file(vert_filename, "vert", "rt", window_get_os_data(window_data));
    if(vert_fp){
        FILE *frag_fp = open_file(frag_filename, "frag", "rt", window_get_os_data(window_data));
        if(frag_fp){
            fseek(vert_fp, 0, SEEK_END);
            fseek(frag_fp, 0, SEEK_END);
            size_t vert_len = ftell(vert_fp);
            size_t frag_len = ftell(frag_fp);
            rewind(vert_fp);
            rewind(frag_fp);
            char *vert_source = calloc(vert_len+1 + frag_len+1, 1);
            char *frag_source = vert_source + vert_len + 1;
            fread(vert_source, vert_len, 1, vert_fp);
            fread(frag_source, frag_len, 1, frag_fp);

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
                print_shader_listing(vert_source, vert_filename, "Vertex");
                goto cleanup;
            }
            glCompileShader(frag_handle);
            if(check_compilation(frag_handle,GL_COMPILE_STATUS,
                error_buffer,error_buffer_len,"fragment shader")!=GL_TRUE)
            {
                print_shader_listing(frag_source, frag_filename, "Fragment");
                goto cleanup;
            }
            glAttachShader(shader_handle,vert_handle);
            glAttachShader(shader_handle,frag_handle);
            glLinkProgram(shader_handle);
            if(check_compilation(shader_handle,GL_LINK_STATUS,
                error_buffer,error_buffer_len,"shader program")!=GL_TRUE)
            {
                goto cleanup;
            }

            shader->valid  = 1;
            shader->handle = shader_handle;

cleanup:
            //NOTE(Vidar):We only need one free since the buffers are allocated together
            free(vert_source);
            fclose(frag_fp);
        }
        fclose(vert_fp);
    }else{
        printf("Could not open file\n");
    }
    

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
    mesh->num_verts = num_verts;

    glGenBuffers(1,&mesh->index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh->index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,num_indices*4,index_data,
                 GL_STATIC_DRAW);

    return mesh;
}

void graphics_update_mesh_verts(struct GraphicsValueSpec* value_specs, uint32_t num_value_specs, struct Mesh *mesh, struct GraphicsData* graphics)
{
    int num_verts = mesh->num_verts;
    for (int i = 0; i < num_value_specs; i++) {
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffers[i]);
        unsigned char* buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        if (buffer) {
			struct GraphicsValueSpec spec = value_specs[i];
			int data_len = graphics_value_sizes[spec.type] * num_verts;
			memcpy(buffer, spec.data, data_len);
			buffer += data_len;
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }
        else {
            GLenum err;
            while ((err = glGetError()) != GL_NO_ERROR)
            {
                printf("OpenGL error %d\n", err);
            }
        }
    }
}

struct Texture *graphics_create_texture(uint8_t *texture_data, uint32_t w, uint32_t h, uint32_t format, struct GraphicsData *graphics)
{
    struct Texture *texture = calloc(1,sizeof(struct Texture));
	glGenTextures(1, &texture->handle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	GLenum gl_format;
	switch (format) {
	case GRAPHICS_PIXEL_FORMAT_RGBA:
		gl_format = GL_RGBA;
		break;
	default:
		gl_format = GL_RGBA;
		break;
	}
    texture->gl_format = gl_format;
    glTexImage2D(GL_TEXTURE_2D, 0,gl_format, w, h,
        0, gl_format, GL_UNSIGNED_BYTE, texture_data);
    printf("Loaded texture!\n");
    return texture;
}

void graphics_update_texture(struct Texture* tex, uint8_t* texture_data,
    uint32_t x, uint32_t y, uint32_t w, uint32_t h, struct GraphicsData* graphics)
{
    glBindTexture(GL_TEXTURE_2D, tex->handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, tex->gl_format , GL_UNSIGNED_BYTE , texture_data);
}

void graphics_set_depth_test(int enabled, struct GraphicsData *graphics)
{
    if (enabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}
  
void graphics_set_scissor_state(int enabled, int x1, int y1, int x2, int y2)
{
    if (enabled) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(x1, y1, x2 - x1, y2 - y1);
    }
    else {
        glDisable(GL_SCISSOR_TEST);
    }
}
