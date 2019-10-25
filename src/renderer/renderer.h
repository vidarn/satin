#pragma once


struct GraphicsData
;
struct Shader
;
struct Mesh
;
struct Texture
;

#define GRAPHICS_VALUE_TYPES \
GRAPHICS_VALUE_TYPE(INT,    1,sizeof(int))\
GRAPHICS_VALUE_TYPE(FLOAT,  1,sizeof(float))\
GRAPHICS_VALUE_TYPE(MAT4,  16,sizeof(float))\
GRAPHICS_VALUE_TYPE(MAT3,   9,sizeof(float))\
GRAPHICS_VALUE_TYPE(VEC4,   4,sizeof(float))\
GRAPHICS_VALUE_TYPE(VEC3,   3,sizeof(float))\
GRAPHICS_VALUE_TYPE(VEC2,   2,sizeof(float))\
GRAPHICS_VALUE_TYPE(HALF,   1,            2)\
GRAPHICS_VALUE_TYPE(HALF2,  2,            2)\
GRAPHICS_VALUE_TYPE(HALF3,  3,            2)\
GRAPHICS_VALUE_TYPE(TEX2,   1,sizeof(struct Texture))

enum GraphicsValueType{
#define GRAPHICS_VALUE_TYPE(name,num,size) GRAPHICS_VALUE_##name,
    GRAPHICS_VALUE_TYPES
#undef GRAPHICS_VALUE_TYPE
};
struct GraphicsValueSpec{
    char *name;
    void *data;
    enum GraphicsValueType type;
    int num;
};

enum GraphicsPixelFormat{
    GRAPHICS_PIXEL_FORMAT_RGBA,
    GRAPHICS_PIXEL_FORMAT_COUNT
};

extern int graphics_value_sizes[];
extern int graphics_value_nums[];



struct GraphicsData *graphics_create(void * param)
;
void graphics_begin_render_pass(float *clear_rgba, struct GraphicsData *graphics)
;
void graphics_end_render_pass(struct GraphicsData *graphics)
;
void graphics_clear(struct GraphicsData *graphics)
;
void graphics_render_mesh(struct Mesh *mesh, struct Shader *shader, struct GraphicsValueSpec *uniform_specs, int num_uniform_specs, struct GraphicsData *graphics)
;

struct Shader *graphics_compile_shader(const char *vert_filename, const char *frag_filename, char *error_buffer, int error_buffer_len, struct GraphicsData *graphics)
;

struct Mesh *graphics_create_mesh(struct GraphicsValueSpec *value_specs, uint32_t num_value_specs, uint32_t num_verts, int *index_data, uint32_t num_indices, struct GraphicsData *graphics)
;

struct Texture *graphics_create_texture(uint8_t *texture_data, uint32_t w, uint32_t h, uint32_t format, struct GraphicsData *graphics)
;
