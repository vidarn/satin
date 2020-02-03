#include "engine.h"
#define STB_IMAGE_IMPLEMENTATION
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wextra"
#include "stb_image.h"
#pragma clang diagnostic pop
#else
#include "stb_image.h"
#endif
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"
#include "sort/sort.h"
#include "renderer/renderer.h"
#include "window/window.h"
#include "os/os.h"
#include "memory/buffer.h"

//#include <immintrin.h>

struct SpriteData{
    int w,h;
    int x,y;
	int should_be_packed;
    unsigned char* data;
};

struct Sprite{
    float width;
    float inv_aspect;
    int texture;
    float uv_offset[2];
    float uv_size[2];
};

struct Mesh;

struct Font{
    struct Sprite sprites[128]; //ASCII characters...
    stbtt_bakedchar baked_chars[128];
    int texture;
    int first_char;
    int last_char;
    float height;
};

//TODO(Vidar):This and the RenderPass have confusing names...
struct RenderTexture{
    unsigned int texture;
    unsigned int framebuffer;
    int width, height;
    struct Sprite sprite;
};

struct ScissorState {
	int enabled;
	float x1, y1;
	float x2, y2;
};

struct GameData{
    //TODO(Vidar):Remove these...
    uint32_t line_vertex_array;
    uint32_t line_vertex_buffer;
	uint32_t sprite_vertex_array;
	uint32_t sprite_vertex_buffer;
    int sprite_shader;
    int line_shader;
	//uint32_t quad_vertex_array;
	//uint32_t quad_vertex_buffer;
    struct Mesh *quad_mesh;
    struct Texture **textures;
    int num_textures;
    struct Shader **shaders;
    int num_shaders;
    struct Mesh **meshes;
    int num_meshes;
    struct Sprite *sprites;
    struct SpriteData *sprite_data;
    int num_sprites;
    struct Sprite *images;
    int num_images;
    struct Font *fonts;
    int num_fonts;
    struct RenderTexture *render_textures;
    int num_render_textures;
    int active_game_state;
    int next_game_state;
    int ticks[64];
    struct FrameData *frame_data;
    int num_game_state_types;
    struct GameState *game_state_types;
    int num_game_states;
    struct GameState *game_states;
    void *common_data;
    float window_min_x, window_max_x;
    float window_min_y, window_max_y;
	int lock_cursor;
    struct WindowData *window_data;
	int debug_mode;
    struct GraphicsData *graphics;
};

int hero_sprite;
int tile_sprite;
int atlas_sprite;

int desk_image;
int console_font;

int screen_render_texture;

int test_mesh;

static const int font_bitmap_size = 512;
const float quad_render_data[] = {
    0.f, 0.f,
    1.f, 0.f,
    1.f, 1.f,
    0.f, 1.f,
};
const int quad_render_index[] = {
    0, 1, 2, 2, 3, 0
};

void render_quad(int shader, struct Matrix3 m, struct GraphicsValueSpec *uniforms,
    int num_uniforms, struct RenderContext *context)
{
    struct RenderQuadList * list = &context->frame_data->render_quad_list;
    while(list->num >= render_sprite_list_size){
        if(list->next == 0){
            list->next = calloc(1,sizeof(struct RenderQuadList));
        }
        list = list->next;
    }
    struct RenderQuad * r = list->quads + list->num;
    list->num++;
    num_uniforms++; //NOTE(Vidar): Reserve space for the transform matrix;
	*(struct Matrix3*)&r->m = multiply_matrix3(m, context->camera_2d);
    r->num_uniforms = num_uniforms;
    //TODO(Vidar):Perhaps use a fixed size buffer to avoid this allocation
    r->uniforms = calloc(num_uniforms*sizeof(struct GraphicsValueSpec),1);
    r->shader = shader;
    for(int i=0;i<num_uniforms-1;i++){
        enum GraphicsValueType type = uniforms[i].type;
        int size = graphics_value_sizes[type]*uniforms[i].num;
        r->uniforms[i+1].name = strdup(uniforms[i].name);
        r->uniforms[i+1].type = type;
        r->uniforms[i+1].num  = uniforms[i].num;
        //TODO(Vidar):We can allocate a single buffer for all uniforms and their names
        r->uniforms[i+1].data = calloc(1,size);
        memcpy(r->uniforms[i+1].data, uniforms[i].data, size);
    }
    {
        r->uniforms[0].name = "matrix";
        r->uniforms[0].num = 1;
        r->uniforms[0].type = GRAPHICS_VALUE_MAT3;
        r->uniforms[0].data = r->m;
    }
    /*
    r->blend_mode = context->blend_mode;
	r->scissor_state = buffer_len(context->frame_data->scissor_states) / sizeof(struct ScissorState) - 1;
    */
}

//TODO(Vidar):Should we be able to specity the anchor point when rendering
// stuff?

static void render_sprite_screen_internal(struct Sprite *sprite,float x, float y,
    float override_width, float width, struct Color color, int shader,
    struct GraphicsValueSpec *uniforms_arg, int num_uniforms_arg,
    struct RenderContext *context)
{
    float sprite_scale = 1.f/(float)reference_resolution;
    float w = width*sprite_scale*0.5f;
    float h = w*sprite->inv_aspect;
    struct Matrix3 m= {
        w, 0.f, 0.f,
        0.f, h, 0.f,
        x+context->offset_x, y+context->offset_y, 1.0f,
    };
    struct GraphicsValueSpec uniforms[32] = {
        {"color",   &color,     GRAPHICS_VALUE_VEC4, 1},
        {"sprite_uv",  sprite->uv_offset,    GRAPHICS_VALUE_VEC4, 1},
        {"sprite",  context->data->textures[sprite->texture],    GRAPHICS_VALUE_TEX2, 1},
    };
    int num_uniforms = 3;
    for(int i=0;i<num_uniforms_arg;i++){
        uniforms[i+num_uniforms] = uniforms_arg[i];
    }
    num_uniforms += num_uniforms_arg;
    render_quad(shader, m, uniforms, num_uniforms, context);
}

void render_sprite_screen(int sprite,float x, float y,
    struct RenderContext *context)
{
    struct Color color = {1.f, 1.f, 1.f, 1.f};
    render_sprite_screen_internal(context->data->sprites + sprite,x,y,0.f,
        context->data->sprites[sprite].width,
        color,context->data->sprite_shader, 0, 0, context);
}

void render_sprite_screen_scaled(int sprite,float x, float y, float scale,
    struct RenderContext *context)
{
    struct Color color = {1.f, 1.f, 1.f, 1.f};
    float width = context->data->sprites[sprite].width *scale;
    render_sprite_screen_internal(context->data->sprites + sprite,x,y,1.f,width,
        color,context->data->sprite_shader, 0, 0,context);
}

void render_sprite_screen_scaled_with_shader(int sprite,float x, float y,
    float scale, int shader, struct GraphicsValueSpec *uniforms, int num_uniforms,
    struct RenderContext *context)
{
    struct Color color = {1.f, 1.f, 1.f, 1.f};
    float width = context->data->sprites[sprite].width *scale;
    render_sprite_screen_internal(context->data->sprites + sprite,x,y,1.f,
        width, color,shader,uniforms,num_uniforms,context);
}

//TODO(Vidar):These could return a vec2 instead of taking a pointer...
void render_char_screen(int char_index, int font_id, float *x, float *y,
    struct Color color, struct RenderContext *context)
{
    struct GameData *data = context->data;
    struct Font *font = data->fonts + font_id;
	if (char_index >= font->first_char && char_index <= font->last_char) {
		struct Sprite *sprite = font->sprites + char_index;
		if (sprite) {
			stbtt_aligned_quad quad = { 0 };
			float fx = 0.f;
			float fy = 0.f;
			stbtt_GetBakedQuad(font->baked_chars, font_bitmap_size, font_bitmap_size,
				char_index, &fx, &fy, &quad, 0);
			//TODO(Vidar):Fix the vertical positioning, should be able to
			// use the quad
			float offset_y = -quad.y1*0.5f / (float)reference_resolution;
			float offset_x = quad.x0*0.5f / (float)reference_resolution;
			render_sprite_screen_internal(sprite, *x + offset_x, *y + offset_y, 0.f, sprite->width,
				color,context->data->sprite_shader, 0, 0, context);
			*x += fx * 0.5f / (float)reference_resolution;
			*y += fy * 0.5f / (float)reference_resolution;
		}
	}
}


void render_string_screen_n(const char *string, int n, int font, float *x,
    float *y, struct Color color, struct RenderContext *context)
{
    for(int i=0; i<n; i++){
        render_char_screen(string[i],font,x,y,color,context);
    }
}

void render_string_screen(const char *string, int font, float *x, float *y,
    struct Color color, struct RenderContext *context)
{
    int len = (int)strlen(string);
    render_string_screen_n(string,len,font,x,y,color,context);
}

void printf_screen(const char *format, int font, float x, float y,
    struct Color color, struct RenderContext *context, ...)
{
    va_list args;
    va_start(args, context);
    int len = vsnprintf(0,0,format,args);
    char *buffer = calloc(len+1, sizeof(char));
    va_start(args, context);
    vsnprintf(buffer,len+1,format,args);
    render_string_screen_n(buffer,len,font,&x,&y,color,context);
    free(buffer);
    va_end(args);
}

float get_string_render_width(int font_id, const char *text, int len,
    struct GameData *data)
{
    struct Font *font = data->fonts + font_id;
    float ret = 0.f;
    for(int i=0; i<len || (len==-1 && text[i] != 0); i++){
        stbtt_aligned_quad quad = {0};
        float fx = 0.f;
        float fy = 0.f;
        stbtt_GetBakedQuad(font->baked_chars, font_bitmap_size, font_bitmap_size,
                           text[i], &fx, &fy, &quad, 0);
        ret+=fx*0.5f/(float)reference_resolution;
    }
    return ret;
}

//TODO(Vidar):Support other alignment modes...
void render_string_screen_fancy(const char *string, int font_id, float *x, float *y,
    float anchor_y, float width, struct Color color, struct RenderContext *context)
{
    {
        //TODO(Vidar):Actually use this???
        struct RenderString rs = {0};
        rs.str = strdup(string);
        rs.font = font_id;
        rs.x = 2.f*(context->w*(*x) + context->offset_x)-1.f;
        rs.y = 2.f*(context->h*(*y) + context->offset_y)-1.f;
        struct RenderStringList * rsl = &context->frame_data->render_string_list;
        while(rsl->num >= render_sprite_list_size){
            if(rsl->next == 0){
                rsl->next = calloc(1,sizeof(struct RenderStringList));
            }
            rsl = rsl->next;
        }
        rsl->strings[rsl->num] = rs;
        rsl->num++;
    }
    struct GameData *data = context->data;
    //TODO(Vidar):Get this from the font somehow
    float line_step_y = 1.1f * 32.f*0.5f/(float)reference_resolution;
    float current_x = *x;
    float current_y = *y - line_step_y*anchor_y;
    struct Font *font = data->fonts + font_id;
    int len = (int)strlen(string);
    int current_line_chars = 0;
    int current_word_chars = 0;
    float current_line_length = 0.f;
    float current_word_length = 0.f;
    int char_offset = 0;
    //printf("===\n");
    for(int i=1; i<len; i++){
        //TODO(Vidar):It is probably easier to just get this directly from 
        // the baked chars...
        stbtt_aligned_quad quad = {0};
        float fx = 0.f;
        float fy = 0.f;
        stbtt_GetBakedQuad(font->baked_chars, font_bitmap_size, font_bitmap_size,
            string[i], &fx, &fy, &quad, 0);
        current_word_chars++;
        current_word_length+=fx*0.5f/(float)reference_resolution;
        if(string[i] == ' '){
            //printf(" is a space");
            if(current_word_length + current_line_length < width){
                current_line_length += current_word_length;
                current_line_chars += current_word_chars;
                current_word_chars = 0;
                current_word_length = 0;
            }else{
                // Line was too long
                //TODO(Vidar):Break it (just break for now ;)
                render_string_screen_n(string+char_offset, current_line_chars,
                    font_id, &current_x,&current_y, color, context);
                current_x = *x;
                current_y -= line_step_y;
                char_offset += current_line_chars+1;
                current_line_length = 0.f;
                current_line_chars = -1;
            }
        }
    }
    render_string_screen_n(string+char_offset, len - char_offset,
        font_id, &current_x,&current_y, color, context);
}


void render_line_screen(float x1, float y1, float x2, float y2, float thickness,
                        struct Color color, struct RenderContext *context)
{
	float m11 = context->camera_2d.m[0];
	float m21 = context->camera_2d.m[1];
	float m12 = context->camera_2d.m[3];
	float m22 = context->camera_2d.m[4];
	float scale_x = sqrtf(m11*m11 + m12*m12);
	float scale_y = sqrtf(m21*m21 + m22*m22);
    float dx1 = (x2-x1);
    float dy1 = (y2-y1);
    float mag = sqrtf(dx1*dx1 + dy1*dy1);
    float dx2 = -dy1/mag*thickness/scale_x;
    float dy2 =  dx1/mag*thickness/scale_y;
    struct Matrix3 m= {
        dx1, dy1, 0.f,
        dx2, dy2, 0.f,
        x1+context->offset_x, y1+context->offset_y, 1.0f,
    };
    struct GraphicsValueSpec uniforms[] = {
        {"color", &color, GRAPHICS_VALUE_VEC4, 1},
    };
    int num_uniforms = sizeof(uniforms)/sizeof(*uniforms);
    render_quad(context->data->line_shader, m, uniforms, num_uniforms, context);
}

void render_rect_screen(float x1, float y1, float x2, float y2, float thickness,
    struct Color color, struct RenderContext *context)
{
    render_line_screen(x1, y1, x1, y2, thickness, color, context);
    render_line_screen(x2, y1, x2, y2, thickness, color, context);
    render_line_screen(x1, y1, x2, y1, thickness, color, context);
    render_line_screen(x1, y2, x2, y2, thickness, color, context);
}

void render_mesh(int mesh, int shader, struct Matrix4 mat, struct GraphicsValueSpec *uniforms,
	int num_uniforms, struct RenderContext *context)
{
	render_mesh_with_callback(mesh, shader, mat, uniforms, num_uniforms, 0, 0, context);
}

void render_mesh_with_callback(int mesh, int shader, struct Matrix4 mat, struct GraphicsValueSpec *uniforms,
    int num_uniforms, void(*callback)(void *param), void *callback_param, struct RenderContext *context)
{
    struct RenderMesh render_mesh = {0};
	render_mesh.callback = callback;
	render_mesh.callback_param = callback_param;
	render_mesh.depth_test = !context->disable_depth_test;
    render_mesh.mesh = mesh;
    render_mesh.shader = shader;
    num_uniforms += 2;
	//render_mesh.scissor_state = buffer_len(context->frame_data->scissor_states) / sizeof(struct ScissorState) - 1;
    *(struct Matrix4*)&render_mesh.m = mat;
    *(struct Matrix4*)&render_mesh.cam = context->camera_3d;
    render_mesh.num_uniforms = num_uniforms;
    //TODO(Vidar):Can we avoid allocating memory??
    render_mesh.uniforms = calloc(num_uniforms*sizeof(struct GraphicsValueSpec),1);
    for(int i=0;i<num_uniforms-2;i++){
        enum GraphicsValueType type = uniforms[i].type;
        int size = graphics_value_sizes[type]*uniforms[i].num;
        render_mesh.uniforms[i+2].name = strdup(uniforms[i].name);
        render_mesh.uniforms[i+2].type = type;
        render_mesh.uniforms[i+2].num = uniforms[i].num;
        render_mesh.uniforms[i+2].data = calloc(1,size);
        memcpy(render_mesh.uniforms[i+2].data, uniforms[i].data, size);
    }
    
    struct RenderMeshList * rml = &context->frame_data->render_mesh_list;
    while(rml->num >= render_sprite_list_size){
        if(rml->next == 0){
            rml->next = calloc(1,sizeof(struct RenderMeshList));
        }
        rml = rml->next;
    }
    struct RenderMesh *rm = rml->meshes + rml->num;
    *rm = render_mesh;
    rml->num++;
    
    *(struct Matrix4*)&rm->m = mat;
    *(struct Matrix4*)&rm->cam = context->camera_3d;
    rm->uniforms[0].name = "model_matrix";
    rm->uniforms[0].type = GRAPHICS_VALUE_MAT4;
    rm->uniforms[0].num = 1;
    rm->uniforms[0].data = rm->m;
    rm->uniforms[1].name = "view_matrix";
    rm->uniforms[1].type = GRAPHICS_VALUE_MAT4;
    rm->uniforms[1].num = 1;
    rm->uniforms[1].data = rm->cam;
}

float *get_sprite_uv_offset(int sprite, struct GameData *data)
{
    struct Sprite *s = data->sprites + sprite;
    return s->uv_offset;
}

struct Texture *get_sprite_texture(int sprite, struct GameData *data)
{
    struct Sprite *s = data->sprites + sprite;
    return data->textures[s->texture];
}

void set_scissor_state(int enabled, float x1, float y1, float x2, float y2, struct RenderContext* context)
{
	//struct ScissorState state = { enabled, x1, y1, x2, y2 };
	//buffer_add(&state, sizeof(struct ScissorState), context->frame_data->scissor_states);
}

static int add_texture(unsigned char *sprite_data, int sprite_w, int sprite_h,
    int32_t format, struct GameData *data)
{
    int tex_index = data->num_textures;
    data->textures = realloc(data->textures,(++data->num_textures)
        *sizeof(struct Texture *));
    data->textures[tex_index] = graphics_create_texture(sprite_data,sprite_w, sprite_h, format, data->graphics);;
    /* BOOKMARK(Vidar): OpenGL call
	glGenTextures(1, data->texture_ids + data->num_texture_ids);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data->texture_ids[data->num_texture_ids]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0,format, sprite_w, sprite_h,
        0, format, GL_UNSIGNED_BYTE, sprite_data);
     */
    return tex_index;
}

int create_render_texture(int w, int h, int32_t format, struct GameData *data)
{
    return 0;
    //TODO(Vidar): Does not work in opengl ES
    /*
    struct RenderTexture rt = {0};
    glGenFramebuffers(1, &rt.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, rt.framebuffer);
    rt.texture = add_texture(0,w,h,GL_RGB,data);

    int texture_id = data->texture_ids[rt.texture];
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_id, 0);
    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffers);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        printf("Error creating frame buffer!\n");
    }
    rt.width = w;
    rt.height = h;
    rt.sprite.width = w;
    rt.sprite.inv_aspect = (float)h/(float)w;
    rt.sprite.texture = rt.texture;
    rt.sprite.uv_offset[0] = 0.f;
    rt.sprite.uv_offset[1] = 0.f;
    rt.sprite.uv_size[0]   = 1.f;
    rt.sprite.uv_size[1]   = 1.f;
    int ret = data->num_render_textures;
    data->num_render_textures++;
    data->render_textures = realloc(data->render_textures,
            data->num_render_textures*sizeof(struct RenderTexture));
    data->render_textures[ret] = rt;
    return ret;
    */
}

static int compare_tinyobj_vert_index(void *a, void*b, void *data)
{
    tinyobj_vertex_index_t *i1 = a;
    tinyobj_vertex_index_t *i2 = b;
    if(i1->v_idx == i2->v_idx){
        return i1->vn_idx < i2->vn_idx;
    }else{
        return i1->v_idx < i2->v_idx;
    }
}

int load_mesh(const char *name, struct GameData *data)
{
    //TODO(Vidar):Don't use obj!!
    FILE *fp=open_file(name,"obj","rb", window_get_os_data(data->window_data));
    if(fp){
        fseek(fp,0,SEEK_END);
        size_t len=ftell(fp);
        rewind(fp);
        char *buf=calloc(len+2,1);
        fread(buf,1,len,fp);
        fclose(fp);
        buf[len] = '\n';
        buf[len+1] = 0;
        len++;

        tinyobj_attrib_t attrib;
        tinyobj_shape_t* shapes=NULL;
        size_t num_shapes;
        static tinyobj_material_t* materials=NULL;
        static size_t num_materials=0;
        unsigned int flags=TINYOBJ_FLAG_TRIANGULATE;
        tinyobj_parse_obj(&attrib,&shapes,&num_shapes,&materials,&num_materials,
            buf,len,flags);
        free(buf);

        //TODO(Vidar): Copy and sort attrib.faces.
        len = attrib.num_face_num_verts*3*sizeof(tinyobj_vertex_index_t);
        tinyobj_vertex_index_t *faces = calloc(len,1);
        tinyobj_vertex_index_t *faces_tmp = calloc(len,1);
        int *idx = calloc(attrib.num_face_num_verts*3, sizeof(int));
        for(unsigned int i=0;i<attrib.num_face_num_verts*3;i++){
            idx[i] = i;
        }
        int *idx_tmp = calloc(attrib.num_face_num_verts*3, sizeof(int));
        memcpy(faces, attrib.faces, len);
        merge_sort_custom_auxillary(faces, faces_tmp, sizeof(tinyobj_vertex_index_t), idx, idx_tmp, attrib.num_face_num_verts*3, compare_tinyobj_vert_index, 0);
        // Traverse and create a new unique vert each time some index changes
        int num_unique_verts = 1;
        for(unsigned int i=1;i<attrib.num_face_num_verts*3;i++){
            tinyobj_vertex_index_t *t = faces + i;
            tinyobj_vertex_index_t *t_prev = t-1;
            if(t->v_idx != t_prev->v_idx || t->vn_idx != t_prev->vn_idx || t->vt_idx != t_prev->vt_idx){
                num_unique_verts++;
            }
        }
        printf("%d unique verts\n", num_unique_verts);
        // Use the sorted indices to assign the correct verts to faces
        int *vert_to_unique_vert = calloc(attrib.num_face_num_verts*3, sizeof(int));

        int num_verts = num_unique_verts;
        int vertex_data_len=num_verts*3*sizeof(float);
        int normal_data_len=num_verts*3*sizeof(float);
        int uv_map_data_len=num_verts*2*sizeof(float);
        float *vertex_data=calloc(vertex_data_len+normal_data_len+uv_map_data_len,1);
        float *normal_data = vertex_data + num_verts*3;
        float *uv_map_data = normal_data + num_verts*3;
        unsigned int face_i = 0;
        int no_normals = 1;
        for(int i=0;i<num_verts;i++){
            tinyobj_vertex_index_t v = faces[face_i];
            vertex_data[i*3 + 0] = attrib.vertices[v.v_idx*3 + 0];
            vertex_data[i*3 + 1] = attrib.vertices[v.v_idx*3 + 1];
            vertex_data[i*3 + 2] = attrib.vertices[v.v_idx*3 + 2];
            if(v.vn_idx > 0){
                normal_data[i*3 + 0] = attrib.normals[v.vn_idx*3 + 0];
                normal_data[i*3 + 1] = attrib.normals[v.vn_idx*3 + 1];
                normal_data[i*3 + 2] = attrib.normals[v.vn_idx*3 + 2];
                no_normals = 0;
            }
            if(v.vt_idx >= 0){
                uv_map_data[i*2 + 0] = attrib.texcoords[v.vt_idx*2 + 0];
                uv_map_data[i*2 + 1] = attrib.texcoords[v.vt_idx*2 + 1];
            }else{
                uv_map_data[i*2 + 0] = -1.f;
                uv_map_data[i*2 + 1] = -1.f;
            }
            tinyobj_vertex_index_t *v_next = faces+face_i;
            while(face_i < attrib.num_face_num_verts*3
                && v_next->v_idx == v.v_idx && v_next->vn_idx == v.vn_idx
                && v_next->vt_idx == v.vt_idx)
            {
                int orig_vert = idx[face_i];
                vert_to_unique_vert[orig_vert] = i;
                face_i++;
                v_next = faces + face_i;
            }
        }
        
        int num_tris = attrib.num_face_num_verts;
        int index_data_len = num_tris*3*sizeof(int);
        int *index_data=calloc(index_data_len,1);
        for(int i=0;i<num_tris*3;i++){
            index_data[i] = vert_to_unique_vert[i];
        }

        free(faces);
        free(faces_tmp);
        free(idx);
        free(idx_tmp);
        free(vert_to_unique_vert);
        
        if(no_normals){
            calculate_mesh_normals(num_verts, vertex_data, normal_data, num_tris, index_data);
        }
        
        struct GraphicsValueSpec data_specs[] = {
            {"pos", (uint8_t *)vertex_data, GRAPHICS_VALUE_VEC3},
            {"normal", (uint8_t *)normal_data, GRAPHICS_VALUE_VEC3},
            {"uv", (uint8_t *)uv_map_data, GRAPHICS_VALUE_VEC2}
        };
        int num_data_specs = sizeof(data_specs)/sizeof(*data_specs);
        
        int ret =  load_custom_mesh_from_memory(num_verts, num_tris, index_data,
            num_data_specs, data_specs, data);
        
        free(index_data);
        free(vertex_data);
        
        return ret;
        
        /* BOOKMARK(Vidar): OpenGL
        mesh.num_tris = attrib.num_face_num_verts;
        int index_data_len = mesh.num_tris*3*sizeof(int);
        unsigned int *index_data=calloc(index_data_len,1);
        for(int i=0;i<mesh.num_tris*3;i++){
            index_data[i] = vert_to_unique_vert[i];
        }

        free(faces);
        free(faces_tmp);
        free(idx);
        free(idx_tmp);
        
        mesh.num_verts = num_verts;

        
        mesh.shader=data->shaders+shader;
        glGenVertexArrays(1,&mesh.vertex_array);
        glBindVertexArray(mesh.vertex_array);
        glGenBuffers(1,&mesh.vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER,mesh.vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER,vertex_data_len+normal_data_len+uv_map_data_len,vertex_data,
            GL_STATIC_DRAW);
        //TODO(Vidar):We should be able to free it now, right?
        //free(vertex_data);
        int pos_loc=glGetAttribLocation(mesh.shader->shader,"pos");
        if(pos_loc == -1){
            printf("Error: Could not find \"pos\" attribute in shader\n");
        }
        glEnableVertexAttribArray(pos_loc);
        glVertexAttribPointer(pos_loc,3,GL_FLOAT,GL_FALSE,
            0,(void*)0);
        
        int normal_loc=glGetAttribLocation(mesh.shader->shader,"normal");
        if(normal_loc == -1){
            printf("Error: Could not find \"normal\" attribute in shader\n");
        }
        glEnableVertexAttribArray(normal_loc);
        glVertexAttribPointer(normal_loc,3,GL_FLOAT,GL_FALSE,
            0,(void*)(intptr_t)vertex_data_len);
        
        int uv_map_loc=glGetAttribLocation(mesh.shader->shader,"uv_map");
        if(uv_map_loc == -1){
            printf("Error: Could not find \"uv_map\" attribute in shader\n");
        }
        glEnableVertexAttribArray(uv_map_loc);
        glVertexAttribPointer(uv_map_loc,2,GL_FLOAT,GL_FALSE,
                              0,(void*)(intptr_t)(vertex_data_len+normal_data_len));


        glGenBuffers(1,&mesh.index_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh.index_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,index_data_len,index_data,
            GL_STATIC_DRAW);
*/
    } else{
        printf("Could not load mesh \"%s\"\n",name);
    }
    return -1;
}

struct WindowData *get_window_data(struct GameData *data)
{
    return data->window_data;
}

struct OSData *get_os_data(struct GameData *data)
{
    return window_get_os_data(data->window_data);
}

int load_mesh_unit_plane(int shader, struct GameData *data)
{
	struct Vec3 pos_data[] = {
		{0.f, 0.f, 0.f},
		{1.f, 0.f, 0.f},
		{0.f, 1.f, 0.f},
		{1.f, 1.f, 0.f},
	};
	struct Vec3 nor_data[] = {
		{0.f, 0.f, 1.f},
		{0.f, 0.f, 1.f},
		{0.f, 0.f, 1.f},
		{0.f, 0.f, 1.f},
	};
	struct Vec2 uv_data[] = {
		{0.f, 0.f},
		{1.f, 0.f},
		{0.f, 1.f},
		{1.f, 1.f},
	};
	int tri_data[] = {
		0, 1, 2,
		3, 2, 1,
	};
	return load_mesh_from_memory(4, pos_data, nor_data, uv_data, 2, tri_data, shader, data);
}

int load_mesh_from_memory(int num_verts, struct Vec3 *pos_data,
	struct Vec3 *normal_data, struct Vec2 *uv_data, int num_tris,
	int *tri_data, int shader, struct GameData *data)
{
    int mesh_index = data->num_meshes;
    data->meshes = realloc(data->meshes,(++data->num_meshes)
                           *sizeof(struct Mesh *));
	update_mesh_from_memory(mesh_index, num_verts, pos_data, normal_data, uv_data, num_tris, tri_data, shader, data);
    return mesh_index;
}

void update_mesh_from_memory(int mesh_index, int num_verts, struct Vec3 *pos_data,
	struct Vec3 *normal_data, struct Vec2 *uv_data, int num_tris,
	int *tri_data, int shader, struct GameData *data)
{
    int vertex_data_len=num_verts*3*sizeof(float);
    int normal_data_len=num_verts*3*sizeof(float);
    int uv_map_data_len=num_verts*2*sizeof(float);
    float *vertex_data=calloc(vertex_data_len+normal_data_len+uv_map_data_len,1);
    memcpy(vertex_data,pos_data,vertex_data_len);
    memcpy(vertex_data + num_verts*3,normal_data,normal_data_len);
    if(uv_data){
        memcpy(vertex_data + 2*num_verts*3,uv_data,  uv_map_data_len);
    }

    /*BOOKMARK(Vidar): OpenGL
    struct Mesh mesh;
    mesh.shader=data->shaders[shader];

    glGenVertexArrays(1,&mesh.vertex_array);
    glBindVertexArray(mesh.vertex_array);
    glGenBuffers(1,&mesh.vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER,mesh.vertex_buffer);
    
    mesh.num_verts = num_verts;
    
    glBufferData(GL_ARRAY_BUFFER,vertex_data_len+normal_data_len+uv_map_data_len,vertex_data,
                 GL_STATIC_DRAW);
    //TODO(Vidar):We should be able to free it now, right?
    //free(vertex_data);
    int pos_loc=glGetAttribLocation(mesh.shader->shader,"pos");
    if(pos_loc == -1){
        printf("Error: Could not find \"pos\" attribute in shader\n");
    }
    glEnableVertexAttribArray(pos_loc);
    glVertexAttribPointer(pos_loc,3,GL_FLOAT,GL_FALSE,
                          0,(void*)0);
    int normal_loc=glGetAttribLocation(mesh.shader->shader,"normal");
    if(normal_loc == -1){
        printf("Error: Could not find \"normal\" attribute in shader\n");
    }
    glEnableVertexAttribArray(normal_loc);
    glVertexAttribPointer(normal_loc,3,GL_FLOAT,GL_FALSE,
                          0,(void*)(intptr_t)vertex_data_len);
    
    if(uv_data){
        int uv_map_loc=glGetAttribLocation(mesh.shader->shader,"uv_map");
        if(uv_map_loc == -1){
            printf("Error: Could not find \"uv_map\" attribute in shader\n");
        }
        glEnableVertexAttribArray(uv_map_loc);
        glVertexAttribPointer(uv_map_loc,2,GL_FLOAT,GL_FALSE,
                              0,(void*)(intptr_t)(vertex_data_len+normal_data_len));
    }
    
    mesh.num_tris = num_tris;
    int index_data_len = mesh.num_tris*3*sizeof(int);

    glGenBuffers(1,&mesh.index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh.index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,index_data_len,tri_data,
                 GL_STATIC_DRAW);
    
    data->meshes[mesh_index] = mesh;
     */
}

int load_custom_mesh_from_memory(int num_verts, int num_tris,
int *tri_data, int num_data_specs, struct GraphicsValueSpec *data_spec, struct GameData *data)
{
    //BOOKMARK(Vidar):implement mesh creation
    struct Mesh *mesh = graphics_create_mesh(data_spec, num_data_specs, num_verts, tri_data, num_tris*3, data->graphics);

    int mesh_index = data->num_meshes;
    data->meshes = realloc(data->meshes,(++data->num_meshes)
                           *sizeof(struct Mesh *));
    data->meshes[mesh_index] = mesh;
    return mesh_index;
}

void save_mesh_to_file(int mesh, const char *name, const char *ext, struct GameData *data)
{
    /*
	//TODO(Vidar):Only supports OBJ for now
	FILE *fp = open_file(name, ext, "wt",data);
	if (fp) {
		struct Mesh m = data->meshes[mesh];
		int num_verts = m.num_verts;
		struct Vec3 *pos_data = calloc(num_verts, sizeof(struct Vec3));
		get_mesh_vert_data(mesh, pos_data, 0, 0, data);
		for (int i = 0; i < num_verts; i++) {
			struct Vec3 p = pos_data[i];
			fprintf(fp, "v %f %f %f\n", p.x, p.y, p.z);
		}
		int num_tris = m.num_tris;
		int *tri_data = calloc(num_tris, 3 * sizeof(float));
		get_mesh_tri_data(mesh, tri_data, data);
		for (int i = 0; i < num_tris; i++) {
			fprintf(fp, "f %d %d %d\n", tri_data[i * 3 + 0]+1,
				tri_data[i * 3 + 1]+1, tri_data[i * 3 + 2]+1);
		}
		fclose(fp);
	}
     */
}

void calculate_mesh_normals(int num_verts, struct Vec3 *pos_data,
    struct Vec3 *normal_data, int num_tris, int *tri_data)
{
    for(int i=0;i<num_verts;i++){
        normal_data[i] = vec3(0.f, 0.f, 0.f);
    }
    for(int i=0;i<num_tris;i++){
        int i1 = tri_data[i*3+0];
        int i2 = tri_data[i*3+1];
        int i3 = tri_data[i*3+2];
        struct Vec3 p1 = pos_data[i1];
        struct Vec3 p2 = pos_data[i2];
        struct Vec3 p3 = pos_data[i3];
        struct Vec3 u = sub_vec3(p2, p1);
        struct Vec3 v = sub_vec3(p3, p1);
        struct Vec3 n = {
            u.y*v.z - u.z*v.y,
            u.z*v.x - u.x*v.z,
            u.x*v.y - u.y*v.x,
        };
        float m = sqrtf(n.x*n.x + n.y*n.y + n.z*n.z);
        n.x/=m; n.y/=m; n.z/=m;
        for(int d=0;d<3;d++){
            normal_data[i1].m[d] += n.m[d];
            normal_data[i2].m[d] += n.m[d];
            normal_data[i3].m[d] += n.m[d];
        }
    }
    for(int i=0;i<num_verts;i++){
        normal_data[i] = normalize_vec3(normal_data[i]);
    }
}

void update_mesh_verts_from_memory(int mesh, struct Vec3 *pos_data,
    struct Vec3 *normal_data, struct Vec2 *uv_data, struct GameData *data)
{
    /*BOOKMARK(Vidar): OpenGL
    struct Mesh m = data->meshes[mesh];
    glBindBuffer(GL_ARRAY_BUFFER, m.vertex_buffer);
    unsigned char *buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    int vertex_data_len = m.num_verts*sizeof(float)*3;
    int normal_data_len = m.num_verts*sizeof(float)*3;
    int uv_map_data_len = m.num_verts*sizeof(float)*2;
    memcpy(buffer, pos_data, vertex_data_len);
    memcpy(buffer+vertex_data_len, normal_data, normal_data_len);
    if(uv_data){
        memcpy(buffer+vertex_data_len+normal_data_len, uv_data, uv_map_data_len);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
     */
}

void update_custom_mesh_verts_from_memory(int mesh, int num_data_specs, struct GraphicsValueSpec *data_spec,
	struct GameData *data)
{
    /*BOOKMARK(Vidar): OpenGL
    struct Mesh m = data->meshes[mesh];
    glBindBuffer(GL_ARRAY_BUFFER, m.vertex_buffer);
    unsigned char *buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	if (buffer) {
		for (int i = 0; i < num_data_specs; i++) {
			struct CustomGraphicsValueSpec spec = data_spec[i];
			int data_len = uniform_sizes[spec.type] * m.num_verts;
			memcpy(buffer, spec.data, data_len);
			buffer += data_len;
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	else {
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			printf("OpenGL error %d\n",err);
		}
	}
     */
}

int get_mesh_num_verts(int mesh, struct GameData *data)
{
    //return data->meshes[mesh].num_verts;
    return 0;
}

void get_mesh_vert_data(int mesh, struct Vec3 *pos_data, struct Vec3 *normal_data,
    struct Vec2 *uv_data, struct GameData *data)
{
    /*BOOKMARK(Vidar):OpenGL
    struct Mesh m = data->meshes[mesh];
    glBindBuffer(GL_ARRAY_BUFFER, m.vertex_buffer);
    unsigned char *buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
    int vertex_data_len = m.num_verts*sizeof(float)*3;
    int normal_data_len = m.num_verts*sizeof(float)*3;
    int uv_map_data_len = m.num_verts*sizeof(float)*2;
    memcpy(pos_data,    buffer,                 vertex_data_len);
    if(normal_data){
        memcpy(normal_data, buffer+vertex_data_len, normal_data_len);
    }
    if(uv_data){
        memcpy(uv_data, buffer+vertex_data_len+normal_data_len, uv_map_data_len);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
     */
}

int get_mesh_num_tris(int mesh, struct GameData *data)
{
    //return data->meshes[mesh].num_tris;
    return 0;
}

void get_mesh_tri_data(int mesh, int *tri_data, struct GameData *data)
{
    /*BOOKMARK(Vidar): OpenGL
    struct Mesh m = data->meshes[mesh];
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.index_buffer);
    unsigned char *buffer = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);
    int data_len = m.num_tris*sizeof(int)*3;
    memcpy(tri_data, buffer, data_len);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
     */
}

unsigned int get_mesh_vertex_buffer(int mesh, struct GameData *data)
{
    /*struct Mesh m = data->meshes[mesh];
	return m.vertex_buffer;*/
    return 0;
}

#define ERROR_BUFFER_LEN 512

int load_shader(const char* vert_filename,
    const char * frag_filename, enum GraphicsBlendMode blend_mode, struct GameData *data)
{
    char error_buffer[ERROR_BUFFER_LEN] = {0};
    struct Shader *shader = graphics_compile_shader(vert_filename, frag_filename, blend_mode, error_buffer, ERROR_BUFFER_LEN, data->graphics, data->window_data);
    if(error_buffer[0] != 0){
        printf("Shader compilation error:\n%s\n",error_buffer);
    }
    data->num_shaders++;
    data->shaders = realloc(data->shaders,data->num_shaders*sizeof(struct Shader *));
    data->shaders[data->num_shaders-1] = shader;
    return data->num_shaders - 1;
}

int load_shader_from_string(const char* vert_source, const char * frag_source,
    struct GameData *data)
{
    /*BOOKMARK(Vidar): OpenGL
    char error_buffer[ERROR_BUFFER_LEN] = {0};
	struct Shader shader = { 0 };
	shader.shader = compile_shader(vert_source,
        frag_source, error_buffer, ERROR_BUFFER_LEN, "#version 330\n");
    if(error_buffer[0] != 0){
        printf("Shader compilation error:\n%s\n",error_buffer);
    }
    data->num_shaders++;
    data->shaders = realloc(data->shaders,data->num_shaders*sizeof(struct Shader));
    data->shaders[data->num_shaders-1] = shader;
    return data->num_shaders - 1;
     */
    return 0;
}

int load_image_from_memory(int sprite_w, int sprite_h,
	unsigned char *sprite_data, struct GameData *data)
{
    int texture = add_texture(sprite_data, sprite_w, sprite_h,GRAPHICS_PIXEL_FORMAT_RGBA,data);
    struct Sprite s;
    s.uv_offset[0] = 0.f;
    s.uv_offset[1] = 0.f;
    s.uv_size[0]   = 1.f;
    s.uv_size[1]   = 1.f;
    s.texture      = texture;
    s.width        = (float)sprite_w;
    s.inv_aspect   = (float)sprite_h/(float)sprite_w;

    //TODO(Vidar): double the amount of sprites instead?
    data->sprites = realloc(data->sprites,(data->num_sprites+1)
        *sizeof(struct Sprite));
    data->sprite_data = realloc(data->sprite_data,(data->num_sprites+1)
        *sizeof(struct SpriteData));
    data->sprites[data->num_sprites] = s;
    struct SpriteData *sd = data->sprite_data + data->num_sprites;
	sd->should_be_packed = 0;
	sd->w = sprite_w;
	sd->h = sprite_h;
	sd->x = 0;
	sd->y = 0;
    data->num_sprites++;

    return data->num_sprites-1;
    return 0;
}

static int load_image_from_fp(FILE* fp, struct GameData* data)
{
    int sprite_channels, sprite_w, sprite_h;
	if (fp) {
		unsigned char* sprite_data = stbi_load_from_file(fp, &sprite_w,
			&sprite_h, &sprite_channels, 4);
		fclose(fp);
		return load_image_from_memory(sprite_w, sprite_h, sprite_data, data);
	}
	return 0;
}

int load_image(const char *name, struct GameData *data)
{
    return load_image_from_fp(open_file(name, "png", "rb",window_get_os_data(data->window_data)),data);
}

int load_image_from_file(const char* filename, struct GameData* data)
{
    return load_image_from_fp(fopen(filename,"rb"),data);
}

void update_sprite_from_memory(int sprite, unsigned char *sprite_data,
    struct GameData *data)
{
    //TODO(Vidar):Update padding as well...
    struct Sprite *s = data->sprites + sprite;
    struct SpriteData *sd = data->sprite_data + sprite;
    /*BOOKMARK(Vidar): OpenGL
    glBindTexture(GL_TEXTURE_2D, data->texture_ids[s->texture]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, sd->x, sd->y, sd->w, sd->h, GL_RGBA,
        GL_UNSIGNED_BYTE, sprite_data);
     */
}

void resize_image_from_memory(int sprite, int sprite_w, int sprite_h, unsigned char *sprite_data,
    struct GameData *data)
{
    struct Sprite *s = data->sprites + sprite;
    s->width        = (float)sprite_w;
    s->inv_aspect   = (float)sprite_h/(float)sprite_w;
    struct SpriteData *sd = data->sprite_data + sprite;
	assert(sd->should_be_packed == 0);
	sd->w = sprite_w;
	sd->h = sprite_h;
	sd->x = 0;
	sd->y = 0;

    /*BOOKMARK(Vidar): OpenGL
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data->texture_ids[s->texture]);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, sprite_w, sprite_h,
        0, GL_RGBA, GL_UNSIGNED_BYTE, sprite_data);
     */
}

int load_sprite_from_memory(int sprite_w, int sprite_h,
    unsigned char *sprite_data, struct GameData *data)
{
    //TODO(Vidar): double the amount of sprites instead?
    data->sprites = realloc(data->sprites,(data->num_sprites+1)
        *sizeof(struct Sprite));
    data->sprite_data = realloc(data->sprite_data,(data->num_sprites+1)
        *sizeof(struct SpriteData));
    struct Sprite *s = data->sprites + data->num_sprites;
    struct SpriteData *sd = data->sprite_data + data->num_sprites;
    int ret = data->num_sprites;

    data->num_sprites++;
    s->inv_aspect = (float)sprite_h/(float)sprite_w;
    s->width = (float)sprite_w;

    sd->w = sprite_w;
    sd->h = sprite_h;
    sd->data = calloc(sd->w * sd->h, 4*sizeof(char));
    memcpy(sd->data,sprite_data,sd->w*sd->h*4);
    sd->should_be_packed = 1;

    printf("Loaded sprite of size %dx%d\n", sprite_w, sprite_h);

    return ret;
}

static int internal_load_sprite_from_fp(FILE *fp, const char *name, struct GameData *data)
{
    int sprite_channels, sprite_w, sprite_h;
	if (fp) {
		unsigned char *sprite_data = stbi_load_from_file(fp, &sprite_w,
			&sprite_h, &sprite_channels, 4);
		fclose(fp);

		if (sprite_data) {
			return load_sprite_from_memory(sprite_w, sprite_h, sprite_data, data);
		}else{
			printf("Warning! Sprite \"%s\" was corrupt\n",name);
			return 0;
		}
    }else{
        printf("Warning! Sprite \"%s\" not found\n",name);
        return 0;
    }
}

int load_sprite(const char *name, struct GameData *data)
{

    //TODO(Vidar):Read from a faster file format??
    //Could use TURBO-jpeg?
    //Or some compression that is supported by the hardware on IOS??

    FILE *fp=open_file(name, "png", "rb",window_get_os_data(data->window_data));
	return internal_load_sprite_from_fp(fp, name, data);
}

int load_sprite_from_filename(const char *filename, struct GameData *data)
{
	FILE *fp = fopen(filename, "rb");
	return internal_load_sprite_from_fp(fp, filename, data);
}


float get_font_height(int font, struct GameData *data)
{
    return data->fonts[font].height*0.25f/(float)reference_resolution;
}

int load_font(const char *name, double font_size, struct GameData *data)
{
    FILE *fp = open_file(name,"ttf","rb",window_get_os_data(data->window_data));
    if(!fp){
        //TODO(Vidar):Report error somehow...
        return -1;
    }
    //TODO(Vidar):This is the ugly version of stb_truetype...
    fseek(fp, 0L, SEEK_END);
    size_t font_len = ftell(fp)+1;
    rewind(fp);
    unsigned char *font_data = calloc(font_len,1);
    fread(font_data,1,font_len,fp);
    fclose(fp);

    data->fonts = realloc(data->fonts, (data->num_fonts+1)*sizeof(struct Font));
    int ret = data->num_fonts;
    struct Font *font = data->fonts+ret;
    data->num_fonts++;

    font->height = (float)font_size;

    const int first_char = 32;
    const int num_chars = 96;
    font->first_char = first_char;
    font->last_char = first_char+num_chars-1;
    unsigned char *font_bitmap = calloc(font_bitmap_size*font_bitmap_size,1);
    stbtt_BakeFontBitmap(font_data,0, (float)font_size, font_bitmap, font_bitmap_size,
        font_bitmap_size, first_char, num_chars, font->baked_chars+first_char);
    free(font_data);
    unsigned char *font_rgba = calloc(font_bitmap_size*font_bitmap_size,4);
    for(int i=0;i<font_bitmap_size*font_bitmap_size;i++){
        font_rgba[i*4+0]=(unsigned char)((float)font_bitmap[i]);
        font_rgba[i*4+1]=(unsigned char)((float)font_bitmap[i]);
        font_rgba[i*4+2]=(unsigned char)((float)font_bitmap[i]);
        font_rgba[i*4+3]=(unsigned char)((float)font_bitmap[i]);
    }

    font->texture =
        add_texture(font_rgba, font_bitmap_size, font_bitmap_size,GRAPHICS_PIXEL_FORMAT_RGBA,data);
    //NOTE(Vidar): Create sprites from each baked_char...
    float inv_w = 1.f/(float)font_bitmap_size;
    float inv_h = 1.f/(float)font_bitmap_size;
    for(int i=0;i<128;i++){
        if(i>=first_char && i < first_char + num_chars){
            struct Sprite s;
            stbtt_bakedchar baked_char = font->baked_chars[i];
            float sprite_w = (float)(baked_char.x1 - baked_char.x0);
            float sprite_h = (float)(baked_char.y1 - baked_char.y0);
            s.uv_offset[0] = (float)baked_char.x0*inv_w;
            s.uv_offset[1] = (float)baked_char.y0*inv_h;
            s.uv_size[0]   = sprite_w*inv_w;
            s.uv_size[1]   = sprite_h*inv_h;
            s.texture      = font->texture;
            s.width        = sprite_w;
            s.inv_aspect   = sprite_h/sprite_w;
            font->sprites[i] = s;
		}
		else {
		}
    }

    free(font_bitmap);
    free(font_rgba);
    return ret;
}

//TODO(Vidar):Pad the sprites
void create_sprite_atlas(struct GameData *data)
{
    int pad = 1;
    //printf("Creating atlas\n");
    //TODO(Vidar):We assume that one 2048x2048 texture is enough for now
    const int w = 4096;
    const int h = 4096;
    #define num_nodes 4096
    stbrp_node nodes[num_nodes];
    stbrp_context context;
    stbrp_init_target(&context,w,h,nodes,num_nodes);
    // TODO(Vidar): call stbrp_allow_out_of_mem() with 'allow_out_of_mem = 1'?
    stbrp_rect *rects = (stbrp_rect*)calloc(data->num_sprites,sizeof(stbrp_rect));
    struct SpriteData *sprite_data = data->sprite_data;
    for(int i=0;i<data->num_sprites;i++){
		if (sprite_data->should_be_packed) {
			//printf("packing sprite %d of size %dx%d\n",i,sprite_data->w,
				//sprite_data->h);
			rects[i].id = i;
			rects[i].w = sprite_data->w + pad * 2;
			rects[i].h = sprite_data->h + pad * 2;
		}
        sprite_data++;
    }
    int status = stbrp_pack_rects(&context,rects,data->num_sprites);
    //printf("Packing status: %d\n",status);
    unsigned char *atlas_data = (unsigned char *)calloc(w*h,4);
    sprite_data = data->sprite_data;
    struct Sprite *sprite = data->sprites;
    float inv_w = 1.f/(float)w;
    float inv_h = 1.f/(float)h;
    for(int i=0;i<data->num_sprites;i++){
		if (sprite_data->should_be_packed) {
			if (rects[i].was_packed) {
				//printf("Sprite %d was packed\n",i);
				int x_offset = rects[i].x + pad;
				int y_offset = rects[i].y + pad;
				sprite_data->x = x_offset;
				sprite_data->y = y_offset;
				for (int iy = -pad; iy < sprite_data->h + pad; iy++) {
					int y = iy;
					if (y < 0) y = 0;
					if (y > sprite_data->h - 1) y = sprite_data->h - 1;
					for (int ix = -pad; ix < sprite_data->w + pad; ix++) {
						int x = ix;
						if (x < 0) x = 0;
						if (x > sprite_data->w - 1) x = sprite_data->w - 1;
						for (int c = 0; c < 4; c++) {
							atlas_data[(ix + x_offset + (iy + y_offset)*w) * 4 + c] =
								sprite_data->data[(x + y * sprite_data->w) * 4 + c];
						}
					}
				}
				sprite->uv_offset[0] = (float)x_offset*inv_w;
				sprite->uv_offset[1] = (float)y_offset*inv_h;
				sprite->uv_size[0] = sprite->width*inv_w;
				sprite->uv_size[1] = sprite->width*sprite->inv_aspect*inv_h;
			}
			else {
				//printf("Sprite %d was not packed\n", i);
			}
		}
        sprite_data++;
        sprite++;
    }
    int texture_id = add_texture(atlas_data,w,h,GRAPHICS_PIXEL_FORMAT_RGBA,data);
    sprite=data->sprites;
	sprite_data = data->sprite_data;
    for(int i=0;i<data->num_sprites;i++){
		if (sprite_data->should_be_packed) {
			sprite->texture = texture_id;
		}
        sprite_data++;
        sprite++;
    }
}

int get_mean_ticks(int length, struct GameData *data)
{
    long accum=0;
    long div = length;
    for(int i=0;i<length;i++){
        if(data->ticks[i] != -1){
            accum += data->ticks[i];
        }else{
            div--;
        }
    }
    return (int)(accum/div);
}

int get_tick_length(struct GameData *data)
{
    return sizeof(data->ticks)/sizeof(*data->ticks);
}

int get_ticks(int i, struct GameData *data)
{
    return data->ticks[i];
}


void get_window_extents(float *x_min, float *x_max, float *y_min, float *y_max,
    struct GameData *data)
{
    *x_min = data->window_min_x;
    *x_max = data->window_max_x;
    *y_min = data->window_min_y;
    *y_max = data->window_max_y;
}

struct FrameData *frame_data_new()
{
    struct FrameData *frame_data = calloc(1,sizeof(struct FrameData));
	//frame_data->scissor_states = buffer_create(512);
    return frame_data;
}

void frame_data_reset(struct FrameData *frame_data)
{
    {
        struct RenderLineList *rll = &frame_data->render_line_list;
        rll->num = 0;
        while(rll->next != 0){
            rll = rll->next;
            rll->num = 0;
        }
    }
    {
        struct RenderSpriteList *rsl = &frame_data->render_sprite_list;
        rsl->num = 0;
        while(rsl->next != 0){
            rsl = rsl->next;
            rsl->num = 0;
        }
    }
    {
        struct RenderStringList *rsl = &frame_data->render_string_list;
        for(int i=0;i<rsl->num;i++){
            free(rsl->strings[i].str);
        }
        while(rsl != 0){
            for(int i=0;i<rsl->num;i++){
                free(rsl->strings[i].str);
            }
            rsl->num = 0;
            rsl = rsl->next;
        }
    }
    {
        struct RenderMeshList *rml = &frame_data->render_mesh_list;
        while(rml != 0){
            for(int i=0;i<rml->num;i++){
                struct RenderMesh *rm = rml->meshes + i;
                //NOTE(Vidar):The first two uniforms are the view and model matrices, don't
                // try to free them!
                for(int i=2;i<rm->num_uniforms;i++){
                    free(rm->uniforms[i].name);
                    free(rm->uniforms[i].data);
                }
                free(rm->uniforms);
            }
            rml->num = 0;
            rml = rml->next;
        }
    }
    {
        struct RenderQuadList *list = &frame_data->render_quad_list;
        while(list != 0){
            for(int i=0;i<list->num;i++){
                struct RenderQuad *r = list->quads + i;
                //NOTE(Vidar):The first uniform is the transform matrix, don't
                // try to free it!
                for(int j=1;j<r->num_uniforms;j++){
                    free(r->uniforms[j].name);
                    free(r->uniforms[j].data);
                }
                free(r->uniforms);
            }
            list->num = 0;
            list = list->next;
        }
    }
	//buffer_reset(frame_data->scissor_states);
    frame_data->clear = 0;
}

void frame_data_clear(struct FrameData *frame_data, struct Color col)
{
    frame_data->clear = 1;
    frame_data->clear_color = col;
}

void set_active_frame_data(struct FrameData *frame_data, struct GameData *data)
{
    data->frame_data = frame_data;
}

struct FrameData *get_active_frame_data(struct GameData *data)
{
    return data->frame_data;
}

int add_game_state(int state_type, struct GameData *data, void *argument)
{
    int num_states = data->num_game_states+1;
    data->game_states = realloc(data->game_states, num_states*sizeof(struct GameState));
    data->game_states[num_states-1] = data->game_state_types[state_type];
    int active_state = data->active_game_state;
    data->active_game_state = num_states-1;
    data->game_states[num_states-1].init(data,argument,active_state);
    data->active_game_state = active_state;
    data->num_game_states = num_states;
    return num_states-1;
}

void remove_game_state(int state, struct GameData *data)
{
    data->game_states[state].should_destroy = 1;
}

void switch_game_state(int state, struct GameData *data)
{
    data->next_game_state = (int)state;
}

int get_active_game_state(struct GameData *data)
{
    return data->active_game_state;
}

void set_custom_data_pointer(void * custom_data, struct GameData *data)
{
	if (data->active_game_state >= 0) {
		data->game_states[data->active_game_state].custom_data = custom_data;
	}
}

void *get_custom_data_pointer(struct GameData *data)
{
    return data->game_states[data->active_game_state].custom_data;
}

void set_common_data_pointer(void * common_data, struct GameData *data)
{
    data->common_data = common_data;
}

void *get_common_data_pointer(struct GameData *data)
{
    return data->common_data;
}

int cursor_locked(struct GameData *data)
{
	return data->lock_cursor;
}

void lock_cursor(struct GameData *data)
{
	data->lock_cursor = 1;
}

void unlock_cursor(struct GameData *data)
{
	data->lock_cursor = 0;
}

int was_key_typed(unsigned int key, struct InputState *input_state)
{
	for (int i = 0; i < input_state->num_keys_typed; i++) {
		if (input_state->keys_typed[i] == key) {
			return 1;
		}
	}
	return 0;
}

int is_key_down(int key) {
	return os_is_key_down(key);
}

float sum_values(float *values, int num)
{
    float naive_sum = 0.f;
    for(int i=0;i<num;i++){
        naive_sum += values[i];
    }

    //NOTE(Vidar):Handle the sum of a large number of objects by summing 2^2=4
    // at a time. Do this in 16 levels, which means that we can sum 2^32 items
    float *accumulation     = alloca(16*sizeof(float));
    int   *accumulation_num = alloca(16*sizeof(int));

    memset(accumulation,    0,16*sizeof(float));
    memset(accumulation_num,0,16*sizeof(int));
    for(int i = 0; i < num; i++) {
        accumulation[0] += values[i];
        accumulation_num[0]++;
        int accum_level = 0;
        while(accumulation_num[accum_level] == (1<<2)){
            accumulation_num[accum_level+1]++;
            accumulation_num[accum_level]=0;
            accumulation[accum_level+1] += accumulation[accum_level];
            accumulation[accum_level] = 0.f;
            accum_level++;
        }
    }
    float sum = 0.f;
    for(int i_accum = 0; i_accum<16; i_accum++){
        sum += accumulation[i_accum];
    }
    return sum;
}


struct GameData *init(int num_game_states, struct GameState *game_states, void *param, struct WindowData *window_data, int debug_mode)
{
    struct GameData *data = calloc(1,sizeof(struct GameData));
    data->window_data = window_data;
    data->graphics = window_get_graphics_data(window_data);
	data->debug_mode = debug_mode;

    data->num_game_state_types = num_game_states;
    data->game_state_types = calloc(num_game_states,sizeof(struct GameState));
	memcpy(data->game_state_types, game_states, num_game_states * sizeof(struct GameState));
    
    data->line_shader = load_shader("line", "line", GRAPHICS_BLEND_MODE_PREMUL , data);
    data->sprite_shader = load_shader("sprite", "sprite", GRAPHICS_BLEND_MODE_PREMUL , data);
    {
        struct GraphicsValueSpec data_specs[] = {
            {"pos", (uint8_t *)quad_render_data, GRAPHICS_VALUE_VEC2}
        };
        int num_data_specs = sizeof(data_specs)/sizeof(*data_specs);
        data->quad_mesh = graphics_create_mesh(data_specs, num_data_specs, 4, (int*)quad_render_index, 6, data->graphics);
    }

	//NOTE(Vidar):Create a default "invalid" sprite so that it is allways sprite 0
	{
		int res = 128;
		uint8_t* pixels = calloc(res * res, 4);
		for (int y = 0; y < res; y++) {
			for (int x = 0; x < res; x++) {
				//uint8_t val = x > y  ^ res-x > y? 255 : 0;
				int delta1 = abs(x - y);
				int delta2 = abs(res - x - y);
				uint8_t val = delta1 < 4 || delta2 < 4 ? 255 : 0;
				int i = x + y * res;
				pixels[i * 4 + 0] = val;
				pixels[i * 4 + 1] = val;
				pixels[i * 4 + 2] = val;
				pixels[i * 4 + 3] = 255;
			}
		}
		load_image_from_memory(res, res, pixels, data);
	}
    
    /* BOOKMARK(Vidar): OpenGL call
    data->sprite_shader = load_shader("sprite" SATIN_SHADER_SUFFIX, "sprite" SATIN_SHADER_SUFFIX, data, "pos", "uv",(char*)0);

	glGenVertexArrays(1,&data->sprite_vertex_array);
	glBindVertexArray(data->sprite_vertex_array);
	glGenBuffers(1,&data->sprite_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER,data->sprite_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,sizeof(sprite_render_data),sprite_render_data,
        GL_DYNAMIC_DRAW);

    {
        int pos_loc=glGetAttribLocation(
            data->shaders[data->sprite_shader].shader, "pos");
        if(pos_loc == -1){
            printf("Error: Could not find \"pos\" attribute in sprite shader\n");
        }
        glEnableVertexAttribArray(pos_loc);
        glVertexAttribPointer(pos_loc,2,GL_FLOAT,GL_FALSE,
            4*sizeof(GL_FLOAT),(void*)0);

        int uv_loc=glGetAttribLocation(
            data->shaders[data->sprite_shader].shader, "uv");
        if(uv_loc == -1){
            printf("Error: Could not find \"uv\" attribute in sprite shader\n");
		}
		else {
			glEnableVertexAttribArray(uv_loc);
			glVertexAttribPointer(uv_loc, 2, GL_FLOAT, GL_FALSE,
				4 * sizeof(GL_FLOAT), (void*)(2 * sizeof(GL_FLOAT)));
		}
    }

	glGenVertexArrays(1,&data->line_vertex_array);
	glBindVertexArray(data->line_vertex_array);
	glGenBuffers(1,&data->line_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER,data->line_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,sizeof(line_render_data),line_render_data,
        GL_DYNAMIC_DRAW);

    {
        int pos_loc=glGetAttribLocation(
            data->shaders[data->line_shader].shader,"pos");
        if(pos_loc == -1){
            printf("Error: Could not find \"pos\" attribute in shader\n");
        }
        glEnableVertexAttribArray(pos_loc);
        glVertexAttribPointer(pos_loc,2,GL_FLOAT,GL_FALSE,
            6*sizeof(GL_FLOAT),(void*)0);
    }

    glActiveTexture(GL_TEXTURE0);

	glGenVertexArrays(1,&data->quad_vertex_array);
	glBindVertexArray(data->quad_vertex_array);
	glGenBuffers(1,&data->quad_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER,data->quad_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,sizeof(quad_render_data),quad_render_data,
        GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0); //"pos" is always at loc 0
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);

    */
	if (num_game_states > 0) {
		data->active_game_state = 0;
		data->game_states = calloc(1, sizeof(struct GameState));
		data->num_game_states = 1;
		data->game_states[0] = data->game_state_types[0];
		data->game_states[0].init(data, param, -1);

		data->active_game_state = -1;
		data->active_game_state = 0;
	}

    create_sprite_atlas(data);


    const int num_ticks = sizeof(data->ticks)/sizeof(*data->ticks);
    for(int i=0;i<num_ticks;i++){
        data->ticks[i] = -1;
    }

    data->active_game_state = 0;

    return data;
}

struct Color rgb(float r, float g, float b)
{
    struct Color ret = {r,g,b,1.f};
    return ret;
}

struct Color rgba(float r, float g, float b, float a)
{
    struct Color ret = {r,g,b,a};
    return ret;
}

struct Color color_white = { 1.f,1.f,1.f,1.f };
struct Color color_black = { 0.f,0.f,0.f,1.f };

void get_sprite_size(int sprite, float *w, float *h, struct GameData *data)
{
    float sprite_scale = 1.f/(float)reference_resolution;
    struct Sprite *s = data->sprites + sprite;
    *w = s->width*sprite_scale*0.5f;
    *h = (*w)*s->inv_aspect;
}

struct Matrix3 get_sprite_matrix(int sprite, struct GameData *data)
{
    struct Sprite *s = data->sprites + sprite;
    float sprite_scale = 1.f/(float)reference_resolution;
    float w = s->width*sprite_scale;
    float h = w*s->inv_aspect;
    struct Matrix3 ret = {
        w, 0.f, 0.f,
        0.f, h, 0.f,
        0.f, 0.f, 1.f,
    };
    return ret;
}

void process_uniforms(int shader, int num_uniforms,
    struct GraphicsValueSpec *uniforms, struct GameData *data)
{
    /* BOOKMARK(Vidar): OpenGL call
    int num_textures = 0;
    for(int j=0;j<num_uniforms;j++){
        struct GraphicsValueSpec u = uniforms[j];
        int loc=glGetUniformLocation(shader,u.name);
        if(loc == -1){
            printf("Warning: Could not find \"%s\" uniform in shader\n",
                u.name);
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
                    int texture_id = *(int*)u.data;
                    glUniform1i(loc, num_textures);
                    glActiveTexture(GL_TEXTURE0 + num_textures);
                    glBindTexture(GL_TEXTURE_2D, data->texture_ids[texture_id]);
                    num_textures++;
                    break;
                }
                case GRAPHICS_VALUE_SPRITE_POINTER:
                case GRAPHICS_VALUE_SPRITE:
                {
                    struct Sprite *sprite;
                    if(u.type == GRAPHICS_VALUE_SPRITE_POINTER){
                        sprite = *(struct Sprite**)u.data;
                    }else{
                        int sprite_id = *(int*)u.data;
                        sprite = data->sprites + sprite_id;
                    }
                    glUniform1i(loc, num_textures);
                    glActiveTexture(GL_TEXTURE0 + num_textures);
                    glBindTexture(GL_TEXTURE_2D,
                        data->texture_ids[sprite->texture]);
                    num_textures++;
                    size_t len = strlen(u.name) + 4;
                    char *buffer = alloca(len);
                    sprintf(buffer, "%s_uv", u.name);
                    int uv_loc=glGetUniformLocation(shader,buffer);
                    if(uv_loc == -1){
                        printf("Error: Could not find \"%s\" uniform in shader\n",
                            buffer);
                    }else{
                        glUniform4fv(uv_loc,1,sprite->uv_offset);
                    }
                    break;
                }
            }
        }
    }
     */
}

void render_meshes(struct FrameData *frame_data, float aspect, int w, int h,
    struct GameData *data)
{
    /* BOOKMARK(Vidar): OpenGL call
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	int current_depth_test_state = 1;
    enum BlendMode blend_mode = BLEND_MODE_NONE;
     */
    graphics_set_depth_test(1,data->graphics);
    struct RenderMeshList *rml = &frame_data->render_mesh_list;
    /*
	int scissor_state = -1;
	struct ScissorState* scissor_states = buffer_ptr(frame_data->scissor_states);
    */
    while(rml != 0){
        for(int i=0;i<rml->num;i++){
            struct RenderMesh *render_mesh = rml->meshes + i;
            struct Mesh *mesh = data->meshes[render_mesh->mesh];
            struct Shader *shader = data->shaders[render_mesh->shader];
            /* BOOKMARK(Vidar): OpenGL call
			if (scissor_state != render_mesh->scissor_state) {
				struct ScissorState s = scissor_states[render_mesh->scissor_state];
				if (s.enabled) {
					glEnable(GL_SCISSOR_TEST);
					float s_w = s.x2 - s.x1;
					float s_h = s.y2 - s.y1;
					if (aspect > 1.f) {
						s.x1 += (aspect - 1.f) * 0.5f;
						s.x1 /= aspect;
						s_w  /= aspect;
					}
					if (aspect < 1.f) {
						s.y1 = (s.y1*aspect +(1.f - aspect) * 0.5f);
						s_h  *= aspect;
					}
					glScissor(
						(int)((float)w * s.x1),
						(int)((float)h * s.y1),
						(int)((float)w * s_w),
						(int)((float)h * s_h)
					);
				}
				else {
					glDisable(GL_SCISSOR_TEST);
				}
				scissor_state = render_mesh->scissor_state;
			}
            int shader = mesh->shader->shader;
            glUseProgram(shader);
            glBindVertexArray(mesh->vertex_array);
            glBindBuffer(GL_ARRAY_BUFFER,mesh->vertex_buffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh->index_buffer);
            int loc_model=glGetUniformLocation(shader,"model_matrix");
            int loc_view=glGetUniformLocation(shader,"view_matrix");
            if(loc_model == -1 || loc_view == -1){
                if(loc_model == -1){
                    printf("Error: Could not find \"%s\" uniform in shader\n",
                           "model_matrix");
                }
                if(loc_view == -1){
                    printf("Error: Could not find \"%s\" uniform in shader\n",
                       "view_matrix");
                }
            }else{
                struct Matrix4 asp_m={0};
                if(aspect<1.f){
                    asp_m.m[0]=1.f;
                    asp_m.m[5]=aspect;
                } else{
                    asp_m.m[0]=1.f/aspect;
                    asp_m.m[5]=1.f;
                }
                asp_m.m[10]=1.f;
                asp_m.m[15]=1.f;
                struct Matrix4 view_m =
                    multiply_matrix4(*(struct Matrix4*)&render_mesh->cam,asp_m);
                struct Matrix4 model_m = *(struct Matrix4*)&render_mesh->m;
                glUniformMatrix4fv(loc_view,1,GL_FALSE,(float*)&view_m.m[0]);
                glUniformMatrix4fv(loc_model,1,GL_FALSE,(float*)&model_m.m[0]);
            }
            process_uniforms(shader, render_mesh->num_uniforms,
                render_mesh->uniforms, data);

			if (current_depth_test_state != render_mesh->depth_test) {
				current_depth_test_state = render_mesh->depth_test;
				if (current_depth_test_state) {
					glEnable(GL_DEPTH_TEST);
				}
				else {
					glDisable(GL_DEPTH_TEST);
				}
			}
			if (render_mesh->callback) {
				render_mesh->callback(render_mesh->callback_param);
			}
            glDrawElements(GL_TRIANGLES, mesh->num_tris*3,GL_UNSIGNED_INT,(void*)0);
             */
            
            graphics_set_depth_test(render_mesh->depth_test, data->graphics);
            struct Matrix4 asp_m={0};
            if(aspect<1.f){
                asp_m.m[0]=1.f;
                asp_m.m[5]=aspect;
            } else{
                asp_m.m[0]=1.f/aspect;
                asp_m.m[5]=1.f;
            }
            asp_m.m[10]=1.f;
            asp_m.m[15]=1.f;
            
            *(struct Matrix4*)&render_mesh->cam = multiply_matrix4(*(struct Matrix4*)&render_mesh->cam,asp_m);
            
            
            if (render_mesh->callback) {
                render_mesh->callback(render_mesh->callback_param);
            }
            graphics_render_mesh(mesh, shader, render_mesh->uniforms,
                     render_mesh->num_uniforms, data->graphics);
        }
        rml = rml->next;
    }
}

void render_quads(struct FrameData *frame_data, float scale, float aspect, int w, int h,
    struct GameData *data)
{
    graphics_set_depth_test(0,data->graphics);
    struct RenderQuadList *list = &frame_data->render_quad_list;
    /* BOOKMARK(Vidar): OpenGL call
    int shader = -1;
    enum BlendMode blend_mode = BLEND_MODE_NONE;
	int scissor_state = -1;
	struct ScissorState* scissor_states = buffer_ptr(frame_data->scissor_states);
	glDisable(GL_SCISSOR_TEST);
    */
    while(list != 0){
        for(int i=0;i<list->num;i++){
            struct RenderQuad *render_quad = list->quads + i;
            struct Shader *quad_shader = data->shaders[render_quad->shader];

            /* BOOKMARK(Vidar): OpenGL call
			if (scissor_state != render_quad.scissor_state) {
				struct ScissorState s = scissor_states[render_quad.scissor_state];
				if (s.enabled) {
					glEnable(GL_SCISSOR_TEST);
					float s_w = s.x2 - s.x1;
					float s_h = s.y2 - s.y1;
					if (aspect > 1.f) {
						s.x1 += (aspect - 1.f) * 0.5f;
						s.x1 /= aspect;
						s_w  /= aspect;
					}
					if (aspect < 1.f) {
						s.y1 = (s.y1*aspect +(1.f - aspect) * 0.5f);
						s_h  *= aspect;
					}
					glScissor(
						(int)((float)w * s.x1),
						(int)((float)h * s.y1),
						(int)((float)w * s_w),
						(int)((float)h * s_h)
					);
				}
				else {
					glDisable(GL_SCISSOR_TEST);
				}
				scissor_state = render_quad.scissor_state;
			}
            glBindVertexArray(data->quad_vertex_array);
            glBindBuffer(GL_ARRAY_BUFFER,data->quad_vertex_buffer);
            int loc=glGetUniformLocation(shader,"matrix");
            if(loc == -1){
                printf("Error: Could not find \"%s\" uniform in shader\n",
                    "matrix");
            }else{
                for(int i=0;i<3;i++){
                    render_quad.m[i*3+0] *= 2.f*scale;
                    render_quad.m[i*3+1] *= 2.f*scale*aspect;
                }
                render_quad.m[6] -= 1.f;
                render_quad.m[7] -= 1.f;
                if(aspect < 1.f){
                    render_quad.m[7] += 1 - scale*aspect;
                }else{
                    render_quad.m[6] += 1 - scale;
                }
                glUniformMatrix3fv(loc,1,GL_FALSE,render_quad.m);
            }
            process_uniforms(shader, render_quad.num_uniforms,
                render_quad.uniforms, data);
            glDrawArrays(GL_TRIANGLES, 0, 6);
          */
            
            for(int i=0;i<3;i++){
                render_quad->m[i*3+0] *= 2.f*scale;
                render_quad->m[i*3+1] *= 2.f*scale*aspect;
            }
            render_quad->m[6] -= 1.f;
            render_quad->m[7] -= 1.f;
            if(aspect < 1.f){
                render_quad->m[7] += 1 - scale*aspect;
            }else{
                render_quad->m[6] += 1 - scale;
            }
            
            graphics_render_mesh(data->quad_mesh, quad_shader, render_quad->uniforms, render_quad->num_uniforms, data->graphics);
        }
        list = list->next;
    }
}

static void render_internal(int w, int h, struct FrameData *frame_data,
    struct GameData *data)
{

    if(frame_data != 0){
        float aspect = (float)w/(float)h;
        float scale=1.f;
        if(aspect>1.f){
            scale=1.f/aspect;
        }
        
     /*BOOKMARK(Vidar):OpenGl
        glViewport(0, 0, w, h);
*/
        graphics_begin_render_pass(&frame_data->clear_color.r, data->graphics);
        if(frame_data->clear){
            graphics_clear(data->graphics);
        }
        
        render_meshes(frame_data,aspect,w,h,data);
        //glDisable(GL_DEPTH_TEST);
        
        render_quads(frame_data,scale,aspect,w,h,data);
    }
    graphics_end_render_pass(data->graphics);
}

void render_to_memory(int w, int h, unsigned char *pixels,
    struct FrameData *frame_data, struct GameData *data)
{
    //BOOKMARK(Vidar): OpenGL
    /*
	float old_window_min_x = data->window_min_x;
	float old_window_max_x = data->window_max_x;
	float old_window_min_y = data->window_min_y;
	float old_window_max_y = data->window_max_y;
    float min_size =
        (float)(w > h ? h : w);
    float pad = fabsf((float)(w - h))*0.5f/min_size;
    data->window_min_x = 0.f; data->window_max_x = 1.f;
    data->window_min_y = 0.f; data->window_max_y = 1.f;
    if(w > h){
        data->window_min_x -= pad;
        data->window_max_x += pad;
    }else{
        data->window_min_y -= pad;
        data->window_max_y += pad;
    }

    uint32_t FramebufferName = 0;
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    uint32_t renderedTexture;
    glGenTextures(1, &renderedTexture);
    glBindTexture(GL_TEXTURE_2D, renderedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(sizeof(DrawBuffers)/sizeof(*DrawBuffers), DrawBuffers);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        printf("Error creating frame buffer!\n");
        return;
    }
    render_internal(w, h, frame_data, data);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	data->window_min_x = old_window_min_x;
	data->window_max_x = old_window_max_x;
	data->window_min_y = old_window_min_y;
	data->window_max_y = old_window_max_y;
    */
}

void render_to_memory_float(int w, int h, float *pixels,
    struct FrameData *frame_data, struct GameData *data)
{
    //BOOKMARK(Vidar): OpenGL
    /*
	float old_window_min_x = data->window_min_x;
	float old_window_max_x = data->window_max_x;
	float old_window_min_y = data->window_min_y;
	float old_window_max_y = data->window_max_y;
    float min_size =
        (float)(w > h ? h : w);
    float pad = fabsf((float)(w - h))*0.5f/min_size;
    data->window_min_x = 0.f; data->window_max_x = 1.f;
    data->window_min_y = 0.f; data->window_max_y = 1.f;
    if(w > h){
        data->window_min_x -= pad;
        data->window_max_x += pad;
    }else{
        data->window_min_y -= pad;
        data->window_max_y += pad;
    }


    uint32_t FramebufferName = 0;
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    uint32_t renderedTexture;
    glGenTextures(1, &renderedTexture);
    glBindTexture(GL_TEXTURE_2D, renderedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(sizeof(DrawBuffers)/sizeof(*DrawBuffers), DrawBuffers);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        printf("Error creating frame buffer!\n");
        return;
    }
    render_internal(w, h, frame_data, data);
	glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteTextures(1, &renderedTexture);
	glDeleteFramebuffers(1, &FramebufferName);

	data->window_min_x = old_window_min_x;
	data->window_max_x = old_window_max_x;
	data->window_min_y = old_window_min_y;
	data->window_max_y = old_window_max_y;
     */
}

void render(int framebuffer_w, int framebuffer_h, struct GameData *data)
{
    //BOOKMARK(Vidar):OpenGL
    
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    render_internal(framebuffer_w, framebuffer_h, data->frame_data, data);
    
    /*float min_size =
        (float)(framebuffer_w > framebuffer_h ? framebuffer_h : framebuffer_w);
    float pad = fabsf((float)(framebuffer_w - framebuffer_h))*0.5f/min_size;
    data->window_min_x = 0.f; data->window_max_x = 1.f;
    data->window_min_y = 0.f; data->window_max_y = 1.f;
    if(framebuffer_w > framebuffer_h){
        data->window_min_x -= pad;
        data->window_max_x += pad;
    }else{
        data->window_min_y -= pad;
        data->window_max_y += pad;
    }
     */
}

int update(int ticks, struct InputState input_state, struct GameData *data)
{
    const int num_ticks = sizeof(data->ticks)/sizeof(*data->ticks);
    for(int i=num_ticks-1; i > 0;i--){
        data->ticks[i] = data->ticks[i-1];
    }
    data->ticks[0] = ticks;
    data->active_game_state = data->next_game_state;
    int ret = data->game_states[data->active_game_state].update(ticks,input_state,data);
    int state = 0;
    while(state<data->num_game_states){
        if(data->game_states[state].should_destroy){
            data->game_states[state].destroy(data);
            for(int i=state;i<data->num_game_states-1;i++){
                data->game_states[i] = data->game_states[i+1];
            }
            data->num_game_states--;
        }else{
            state++;
        }
    }
	return ret;
}

void end_game(struct GameData *data)
{
    for(int i=0;i<data->num_game_states;i++){
        data->game_states[i].destroy(data);
    }
}
