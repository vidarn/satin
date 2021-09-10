#pragma once
#include <stdint.h>


struct GraphicsData
;
struct Shader
;
struct Mesh
;
struct Texture
;
struct RenderPass
;

//TODO(Vidar):struct that is Mesh + Shader???

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


enum GraphicsBlendMode {
    GRAPHICS_BLEND_MODE_PREMUL,
    GRAPHICS_BLEND_MODE_MULTIPLY,
    GRAPHICS_BLEND_MODE_NONE,
};

extern int graphics_value_sizes[];
extern int graphics_value_nums[];



struct GraphicsData *graphics_create(void * param)
;
void graphics_set_viewport(int x, int y, int w, int h)
;
void graphics_begin_render_pass(float *clear_rgba, struct GraphicsData *graphics)
;
void graphics_end_render_pass(struct GraphicsData *graphics)
;
void graphics_clear(struct GraphicsData *graphics)
;

void graphics_render_mesh(struct Mesh *mesh, struct Shader *shader, struct GraphicsValueSpec *uniform_specs, int num_uniform_specs, struct GraphicsData *graphics)
;

struct WindowData;
struct Shader *graphics_compile_shader(const char *vert_source, const char *frag_source,
    enum GraphicsBlendMode blend_mode, char *error_buffer, int error_buffer_len, struct GraphicsData *graphics,
    struct WindowData *window_data)
;

struct Mesh *graphics_create_mesh(struct GraphicsValueSpec *value_specs, uint32_t num_value_specs, uint32_t num_verts, int *index_data, uint32_t num_indices, struct GraphicsData *graphics)
;
void graphics_update_mesh_verts(struct GraphicsValueSpec* value_specs, uint32_t num_value_specs, struct Mesh *mesh, struct GraphicsData* graphics)
;

struct Texture *graphics_create_texture(uint8_t *texture_data, uint32_t w, uint32_t h, uint32_t format, struct GraphicsData *graphics)
;
void graphics_update_texture(struct Texture *tex, uint8_t *texture_data,
    uint32_t x, uint32_t y, uint32_t w, uint32_t h, struct GraphicsData *graphics)
;

void graphics_set_scissor_state(int enabled, int x1, int y1, int x2, int y2)
;

void graphics_set_depth_test(int enabled, struct GraphicsData *graphics)
;
