#include "engine.h"
#include "vn_gl.h"
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
#include "graphics/graphics.h"
#include "sort/sort.h"

//#include <immintrin.h>

static const int uniform_sizes[] = {
#define SHADER_UNIFORM_TYPE(name,num,size,gl_type) num*size,
    SHADER_UNIFORM_TYPES
#undef SHADER_UNIFORM_TYPE
};

static const int uniform_gl_types[] = {
#define SHADER_UNIFORM_TYPE(name,num,size,gl_type) gl_type,
    SHADER_UNIFORM_TYPES
#undef SHADER_UNIFORM_TYPE
};

static const int uniform_nums[] = {
#define SHADER_UNIFORM_TYPE(name,num,size,gl_type) num,
    SHADER_UNIFORM_TYPES
#undef SHADER_UNIFORM_TYPE
};

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

struct Mesh{
    GLuint vertex_array;
    GLuint vertex_buffer;
    GLuint index_buffer;
    int num_tris, num_verts;
    struct Shader *shader;
};

struct Font{
    int texture;
    int first_char;
    float height;
    struct Sprite sprites[128]; //ASCII characters...
    stbtt_bakedchar baked_chars[128];
};

//TODO(Vidar):This and the RenderPass have confusing names...
struct RenderTexture{
    unsigned int texture;
    unsigned int framebuffer;
    int width, height;
    struct Sprite sprite;
};

struct GameData{
    //TODO(Vidar):Remove these...
    GLuint line_vertex_array;
    GLuint line_vertex_buffer;
	GLuint sprite_vertex_array;
	GLuint sprite_vertex_buffer;
    int sprite_shader;
    int line_shader;
	GLuint quad_vertex_array;
	GLuint quad_vertex_buffer;
    GLuint *texture_ids;
    int num_texture_ids;
    struct Shader *shaders;
    int num_shaders;
    struct Mesh *meshes;
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
	void *os_data;
	int debug_mode;
};

int hero_sprite;
int tile_sprite;
int atlas_sprite;

int desk_image;
int console_font;

int screen_render_texture;

int test_mesh;

float sprite_render_data[render_sprite_list_size*6*2*2*2];
float line_render_data[  render_sprite_list_size*6*2*2*2];
static const int font_bitmap_size = 512;
const float quad_render_data[] = {
    0.f, 0.f,
    1.f, 0.f,
    1.f, 1.f,
    1.f, 1.f,
    0.f, 1.f,
    0.f, 0.f,
};

void render_quad(int shader, struct Matrix3 m, struct ShaderUniform *uniforms,
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
	*(struct Matrix3*)&r->m = multiply_matrix3(m, context->camera_2d);
    r->num_uniforms = num_uniforms;
    r->uniforms = calloc(num_uniforms*sizeof(struct ShaderUniform),1);
    r->shader = shader;
    for(int i=0;i<num_uniforms;i++){
        enum ShaderUniformType type = uniforms[i].type;
        int size = uniform_sizes[type]*uniforms[i].num;
        r->uniforms[i].name = strdup(uniforms[i].name);
        r->uniforms[i].type = type;
        r->uniforms[i].num  = uniforms[i].num;
        r->uniforms[i].data = calloc(1,size);
        memcpy(r->uniforms[i].data, uniforms[i].data, size);
    }
}

//TODO(Vidar):Should we be able to specity the anchor point when rendering
// stuff?

static void render_sprite_screen_internal(struct Sprite *sprite,float x, float y,
    float override_width, float width, struct Color color,
    struct RenderContext *context)
{

    /*
    struct RenderSpriteList * rsl = &context->frame_data->render_sprite_list;
    //TODO(Vidar):Handle the case when the list of render lines is full...
    while(rsl->num >= render_sprite_list_size){
        if(rsl->next == 0){
            rsl->next = calloc(1,sizeof(struct RenderSpriteList));
        }
        rsl = rsl->next;
    }
    struct RenderSprite * r = rsl->sprites + rsl->num;
    rsl->num++;
    r->pos[0] = x*2.f-1.f;
    r->pos[1] = y*2.f-1.f;
    r->sprite = sprite;
    r->override_width = override_width;
    r->width = width;
     */
    float sprite_scale = 1.f/(float)reference_resolution;
    float w = width*sprite_scale*0.5f;
    float h = w*sprite->inv_aspect;
    struct Matrix3 m= {
        w, 0.f, 0.f,
        0.f, h, 0.f,
        x+context->offset_x, y+context->offset_y, 1.0f,
    };
    struct ShaderUniform uniforms[] = {
        {"sprite",       SHADER_UNIFORM_SPRITE_POINTER, 1, &sprite},
        {"color",        SHADER_UNIFORM_VEC4, 1, &color},
    };
    int num_uniforms = sizeof(uniforms)/sizeof(*uniforms);
    render_quad(context->data->sprite_shader, m, uniforms, num_uniforms, context);
}

void render_sprite_screen(int sprite,float x, float y,
    struct RenderContext *context)
{
    struct Color color = {1.f, 1.f, 1.f, 1.f};
    render_sprite_screen_internal(context->data->sprites + sprite,x,y,0.f,
        context->data->sprites[sprite].width, color,context);
}

void render_sprite_screen_scaled(int sprite,float x, float y, float scale,
    struct RenderContext *context)
{
    struct Color color = {1.f, 1.f, 1.f, 1.f};
    float width = context->data->sprites[sprite].width *scale;
    render_sprite_screen_internal(context->data->sprites + sprite,x,y,1.f,width,
        color,context);
}

//TODO(Vidar):These could return a vec2 instead of taking a pointer...
void render_char_screen(int char_index, int font_id, float *x, float *y,
    struct Color color, struct RenderContext *context)
{
    struct GameData *data = context->data;
    struct Font *font = data->fonts + font_id;
    struct Sprite *sprite = font->sprites+char_index;
    if(sprite){
        stbtt_aligned_quad quad = {0};
        float fx = 0.f;
        float fy = 0.f;
        stbtt_GetBakedQuad(font->baked_chars, font_bitmap_size, font_bitmap_size,
            char_index, &fx, &fy, &quad, 0);
        //TODO(Vidar):Fix the vertical positioning, should be able to
        // use the quad
        float offset_y = -quad.y1*0.5f/(float)reference_resolution;
        float offset_x = quad.x0*0.5f/(float)reference_resolution;
        render_sprite_screen_internal(sprite,*x+offset_x, *y+offset_y,0.f,sprite->width,
            color, context);
        *x += fx*0.5f/(float)reference_resolution;
        *y += fy*0.5f/(float)reference_resolution;
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

void printf_screen(char *format, int font, float x, float y,
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
    for(int i=0; i<len; i++){
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
	float scale_x = context->camera_2d.m[0];
	float scale_y = context->camera_2d.m[4];
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
    struct ShaderUniform uniforms[] = {
        {"color", SHADER_UNIFORM_VEC4, 1, &color},
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

void render_mesh(int mesh, struct Matrix4 mat, struct ShaderUniform *uniforms,
	int num_uniforms, struct RenderContext *context)
{
	render_mesh_with_callback(mesh, mat, uniforms, num_uniforms, 0, 0, context);
}

void render_mesh_with_callback(int mesh, struct Matrix4 mat, struct ShaderUniform *uniforms,
    int num_uniforms, void(*callback)(void *param), void *callback_param, struct RenderContext *context)
{
    struct RenderMesh render_mesh = {0};
	render_mesh.callback = callback;
	render_mesh.callback_param = callback_param;
	render_mesh.depth_test = !context->disable_depth_test;
    render_mesh.mesh = mesh;
    *(struct Matrix4*)&render_mesh.m = mat;
    *(struct Matrix4*)&render_mesh.cam = context->camera_3d;
    render_mesh.num_uniforms = num_uniforms;
    //TODO(Vidar):Can we avoid allocating memory??
    render_mesh.uniforms = calloc(num_uniforms*sizeof(struct ShaderUniform),1);
    for(int i=0;i<num_uniforms;i++){
        enum ShaderUniformType type = uniforms[i].type;
        int size = uniform_sizes[type]*uniforms[i].num;
        render_mesh.uniforms[i].name = strdup(uniforms[i].name);
        render_mesh.uniforms[i].type = type;
        render_mesh.uniforms[i].num = uniforms[i].num;
        render_mesh.uniforms[i].data = calloc(1,size);
        memcpy(render_mesh.uniforms[i].data, uniforms[i].data, size);
    }
    
    struct RenderMeshList * rml = &context->frame_data->render_mesh_list;
    while(rml->num >= render_sprite_list_size){
        if(rml->next == 0){
            rml->next = calloc(1,sizeof(struct RenderMeshList));
        }
        rml = rml->next;
    }
    rml->meshes[rml->num] = render_mesh;
    rml->num++;
}

static int add_texture(unsigned char *sprite_data, int sprite_w, int sprite_h,
    GLint format, struct GameData *data)
{
    data->texture_ids = realloc(data->texture_ids, (data->num_texture_ids+1)*
                                sizeof(GLuint));
	glGenTextures(1, data->texture_ids + data->num_texture_ids);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data->texture_ids[data->num_texture_ids]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0,format, sprite_w, sprite_h,
        0, format, GL_UNSIGNED_BYTE, sprite_data);
    data->num_texture_ids++;
    return data->num_texture_ids - 1;
}

int create_render_texture(int w, int h, GLint format, struct GameData *data)
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

int load_mesh(const char *name, int shader,
    struct GameData *data)
{
    //TODO(Vidar):Don't use obj!!
    FILE *fp=open_file(name,".obj","rb",data);
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

        struct Mesh mesh;
        mesh.shader=&(data->shaders[shader]);
        
        
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
        for(int i=0;i<num_verts;i++){
            tinyobj_vertex_index_t v = faces[face_i];
            vertex_data[i*3 + 0] = attrib.vertices[v.v_idx*3 + 0];
            vertex_data[i*3 + 1] = attrib.vertices[v.v_idx*3 + 1];
            vertex_data[i*3 + 2] = attrib.vertices[v.v_idx*3 + 2];
            normal_data[i*3 + 0] = attrib.normals[v.vn_idx*3 + 0];
            normal_data[i*3 + 1] = attrib.normals[v.vn_idx*3 + 1];
            normal_data[i*3 + 2] = attrib.normals[v.vn_idx*3 + 2];
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

        /*
        int num_verts = attrib.num_face_num_verts*3;
        int vertex_data_len=num_verts*3*sizeof(float);
        int normal_data_len=num_verts*3*sizeof(float);
        float *vertex_data=calloc(vertex_data_len+normal_data_len,1);
        float *normal_data = vertex_data + num_verts*3;
        for(int i=0;i<attrib.num_face_num_verts;i++){
            for(int j=0;j<3;j++){
                int a = i*3 + j;
                tinyobj_vertex_index_t v = attrib.faces[a];
                vertex_data[a*3 + 0] = attrib.vertices[v.v_idx*3  + 0];
                vertex_data[a*3 + 1] = attrib.vertices[v.v_idx*3  + 1];
                vertex_data[a*3 + 2] = attrib.vertices[v.v_idx*3  + 2];
                
                normal_data[a*3 + 0] = attrib.normals[v.vn_idx*3 + 0];
                normal_data[a*3 + 1] = attrib.normals[v.vn_idx*3 + 1];
                normal_data[a*3 + 2] = attrib.normals[v.vn_idx*3 + 2];
            }
        }
         */
        
        mesh.num_verts = num_verts;

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

        int mesh_index = data->num_meshes;
        data->meshes = realloc(data->meshes,(++data->num_meshes)
            *sizeof(struct Mesh));
        data->meshes[mesh_index] = mesh;
        return mesh_index;
    } else{
        printf("Could not load mesh \"%s\"\n",name);
    }
    return -1;
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
    int vertex_data_len=num_verts*3*sizeof(float);
    int normal_data_len=num_verts*3*sizeof(float);
    int uv_map_data_len=num_verts*2*sizeof(float);
    float *vertex_data=calloc(vertex_data_len+normal_data_len+uv_map_data_len,1);
    memcpy(vertex_data,pos_data,vertex_data_len);
    memcpy(vertex_data + num_verts*3,normal_data,normal_data_len);
    if(uv_data){
        memcpy(vertex_data + 2*num_verts*3,uv_data,  uv_map_data_len);
    }

    struct Mesh mesh;
    mesh.shader=&(data->shaders[shader]);
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
    
    int mesh_index = data->num_meshes;
    data->meshes = realloc(data->meshes,(++data->num_meshes)
                           *sizeof(struct Mesh));
    data->meshes[mesh_index] = mesh;
    return mesh_index;
}

int load_custom_mesh_from_memory(int num_verts, int num_tris,
    int *tri_data, int shader, struct GameData *data, int num_data_specs, struct CustomMeshDataSpec *data_spec)
{
	int total_data_len = 0;
	for (int i = 0; i < num_data_specs; i++) {
		struct CustomMeshDataSpec spec = data_spec[i];
		total_data_len += uniform_sizes[spec.type]*num_verts;
	}
    unsigned char *vertex_data=calloc(total_data_len,1);
	unsigned char *vd = vertex_data;
	for (int i = 0; i < num_data_specs; i++) {
		struct CustomMeshDataSpec spec = data_spec[i];
		int data_len = uniform_sizes[spec.type]*num_verts;
		memcpy(vd, spec.data, data_len);
		vd += data_len;
	}

    struct Mesh mesh;
    mesh.shader=&(data->shaders[shader]);
    glGenVertexArrays(1,&mesh.vertex_array);
    glBindVertexArray(mesh.vertex_array);
    glGenBuffers(1,&mesh.vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER,mesh.vertex_buffer);
    
    mesh.num_verts = num_verts;
    
    glBufferData(GL_ARRAY_BUFFER,total_data_len,vertex_data,
                 GL_DYNAMIC_DRAW);
    //TODO(Vidar):We should be able to free it now, right?
    //free(vertex_data);
	int offset = 0;
	for (int i = 0; i < num_data_specs; i++) {
		struct CustomMeshDataSpec spec = data_spec[i];
		int loc=glGetAttribLocation(mesh.shader->shader,spec.name);
		if(loc == -1){
			printf("Error: Could not find \"%s\" attribute in shader\n", spec.name);
		}
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc,uniform_nums[spec.type],uniform_gl_types[spec.type],GL_FALSE,
			0, (void*)(intptr_t)(offset));
		offset += uniform_sizes[spec.type]*num_verts;
	}
    
    mesh.num_tris = num_tris;
    int index_data_len = mesh.num_tris*3*sizeof(int);

    glGenBuffers(1,&mesh.index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh.index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,index_data_len,tri_data,
                 GL_STATIC_DRAW);
    
    int mesh_index = data->num_meshes;
    data->meshes = realloc(data->meshes,(++data->num_meshes)
                           *sizeof(struct Mesh));
    data->meshes[mesh_index] = mesh;
    return mesh_index;
}

void save_mesh_to_file(int mesh, const char *name, const char *ext, struct GameData *data)
{
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
}

#pragma optimize("", off)
void update_custom_mesh_verts_from_memory(int mesh, int num_data_specs, struct CustomMeshDataSpec *data_spec,
	struct GameData *data)
{
    struct Mesh m = data->meshes[mesh];
    glBindBuffer(GL_ARRAY_BUFFER, m.vertex_buffer);
    unsigned char *buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	if (buffer) {
		for (int i = 0; i < num_data_specs; i++) {
			struct CustomMeshDataSpec spec = data_spec[i];
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
}
#pragma optimize("", on)

int get_mesh_num_verts(int mesh, struct GameData *data)
{
    return data->meshes[mesh].num_verts;
}

void get_mesh_vert_data(int mesh, struct Vec3 *pos_data, struct Vec3 *normal_data,
    struct Vec2 *uv_data, struct GameData *data)
{
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
}

int get_mesh_num_tris(int mesh, struct GameData *data)
{
    return data->meshes[mesh].num_tris;
}

void get_mesh_tri_data(int mesh, int *tri_data, struct GameData *data)
{
    struct Mesh m = data->meshes[mesh];
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.index_buffer);
    unsigned char *buffer = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);
    int data_len = m.num_tris*sizeof(int)*3;
    memcpy(tri_data, buffer, data_len);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

unsigned int get_mesh_vertex_buffer(int mesh, struct GameData *data)
{
    struct Mesh m = data->meshes[mesh];
	return m.vertex_buffer;
}

#define ERROR_BUFFER_LEN 512

//TODO(Vidar):Support additional parameters...
int load_shader(const char* vert_filename, const char * frag_filename,
    struct GameData *data, ...)
{
    char error_buffer[ERROR_BUFFER_LEN] = {0};
    va_list args;
    va_start(args,data);
    struct Shader shader = compile_shader_filename_vararg(vert_filename,
        frag_filename, error_buffer, ERROR_BUFFER_LEN, "#version 330\n",
        data, args);
    va_end(args);
    if(error_buffer[0] != 0){
        printf("Shader compilation error:\n%s\n",error_buffer);
    }
    data->num_shaders++;
    data->shaders = realloc(data->shaders,data->num_shaders*sizeof(struct Shader));
    data->shaders[data->num_shaders-1] = shader;
    return data->num_shaders - 1;
}

int load_shader_from_string(const char* vert_source, const char * frag_source,
    struct GameData *data)
{
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
}

int load_image_from_memory(int sprite_w, int sprite_h,
	unsigned char *sprite_data, struct GameData *data)
{
    int texture = add_texture(sprite_data, sprite_w, sprite_h,GL_RGBA,data);
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
}

int load_image(const char *name, struct GameData *data)
{
    int sprite_channels, sprite_w, sprite_h;
    FILE *fp=open_file(name, ".png", "rb",data);
    unsigned char *sprite_data = stbi_load_from_file(fp, &sprite_w,
        &sprite_h, &sprite_channels, 4);
    fclose(fp);
	return load_image_from_memory(sprite_w, sprite_h, sprite_data, data);
}

void update_sprite_from_memory(int sprite, unsigned char *sprite_data,
    struct GameData *data)
{
    //TODO(Vidar):Update padding as well...
    struct Sprite *s = data->sprites + sprite;
    struct SpriteData *sd = data->sprite_data + sprite;
    glBindTexture(GL_TEXTURE_2D, data->texture_ids[s->texture]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, sd->x, sd->y, sd->w, sd->h, GL_RGBA,
        GL_UNSIGNED_BYTE, sprite_data);
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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data->texture_ids[s->texture]);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, sprite_w, sprite_h,
        0, GL_RGBA, GL_UNSIGNED_BYTE, sprite_data);
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
    if(fp){
        unsigned char *sprite_data = stbi_load_from_file(fp, &sprite_w,
            &sprite_h, &sprite_channels, 4);
        fclose(fp);
        
        return load_sprite_from_memory(sprite_w, sprite_h, sprite_data, data);
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

    FILE *fp=open_file(name, ".png", "rb",data);
	return internal_load_sprite_from_fp(fp, name, data);
}

int load_sprite_from_filename(const char *filename, struct GameData *data)
{
	FILE *fp = fopen(filename, "rb");
	return internal_load_sprite_from_fp(fp, filename, data);
}


float get_font_height(int font, struct GameData *data)
{
    return data->fonts[font].height*0.25f;
}

int load_font(const char *name, double font_size, struct GameData *data)
{
    FILE *fp = open_file(name,".ttf","rb",data);
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
        add_texture(font_rgba, font_bitmap_size, font_bitmap_size,GL_RGBA,data);
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
    }

    free(font_bitmap);
    free(font_rgba);
    return ret;
}

//TODO(Vidar):Pad the sprites
void create_sprite_atlas(struct GameData *data)
{
    int pad = 1;
    printf("Creating atlas\n");
    //TODO(Vidar):We assume that one 2048x2048 texture is enough for now
    const int w = 2048;
    const int h = 2048;
    #define num_nodes 4096
    stbrp_node nodes[num_nodes];
    stbrp_context context;
    stbrp_init_target(&context,w,h,nodes,num_nodes);
    // TODO(Vidar): call stbrp_allow_out_of_mem() with 'allow_out_of_mem = 1'?
    stbrp_rect *rects = (stbrp_rect*)calloc(data->num_sprites,sizeof(stbrp_rect));
    struct SpriteData *sprite_data = data->sprite_data;
    for(int i=0;i<data->num_sprites;i++){
		if (sprite_data->should_be_packed) {
			printf("packing sprite %d of size %dx%d\n",i,sprite_data->w,
				sprite_data->h);
			rects[i].id = i;
			rects[i].w = sprite_data->w + pad * 2;
			rects[i].h = sprite_data->h + pad * 2;
		}
        sprite_data++;
    }
    int status = stbrp_pack_rects(&context,rects,data->num_sprites);
    printf("Packing status: %d\n",status);
    unsigned char *atlas_data = (unsigned char *)calloc(w*h,4);
    sprite_data = data->sprite_data;
    struct Sprite *sprite = data->sprites;
    float inv_w = 1.f/(float)w;
    float inv_h = 1.f/(float)h;
    for(int i=0;i<data->num_sprites;i++){
	    printf("Sprite %d\n");
		if (sprite_data->should_be_packed) {
			if (rects[i].was_packed) {
				printf("Sprite %d was packed\n",i);
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
				printf("Sprite %d was not packed\n", i);
			}
		}
        sprite_data++;
        sprite++;
    }
    int texture_id = add_texture(atlas_data,w,h,GL_RGBA,data);
    sprite=data->sprites;
	sprite_data = data->sprite_data;
    for(int i=0;i<data->num_sprites;i++){
		if (sprite_data->should_be_packed) {
			sprite->texture = texture_id;
		}
        sprite_data++;
        sprite++;
    }
    printf("Atlas created\n");
    //DEBUG(Vidar):Write atlas to image
#if 0
    stbi_write_png("/tmp/atlas.png",w,h,4,atlas_data,0);
    printf("Atlas written to file\n");
#endif
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
                for(int i=0;i<rm->num_uniforms;i++){
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
                for(int j=0;j<r->num_uniforms;j++){
                    free(r->uniforms[j].name);
                    free(r->uniforms[j].data);
                }
                free(r->uniforms);
            }
            list->num = 0;
            list = list->next;
        }
    }
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

struct Vec2 transform_vector(struct Vec2 p, struct Vec2 x_axis, struct Vec2 y_axis)
{
    struct Vec2 ret = {0};
    ret.x = x_axis.x*p.x + x_axis.y*p.y;
    ret.y = y_axis.x*p.x + y_axis.y*p.y;
    return ret;
}

struct Vec2 untransform_vector(struct Vec2 p, struct Vec2 x_axis, struct Vec2 y_axis)
{
    struct Vec2 ret = {0};
    ret.x = x_axis.x*p.x + y_axis.x*p.y;
    ret.y = x_axis.y*p.x + y_axis.y*p.y;
    return ret;
}

struct Vec3 transform_vec3(struct Vec3 p, struct Vec3 x_axis,
    struct Vec3 y_axis, struct Vec3 z_axis)
{
    struct Vec3 ret = {0};
    ret.x = x_axis.x*p.x + x_axis.y*p.y + x_axis.z*p.z;
    ret.y = y_axis.x*p.x + y_axis.y*p.y + y_axis.z*p.z;
    ret.z = z_axis.x*p.x + z_axis.y*p.y + z_axis.z*p.z;
    return ret;
}

struct Vec3 untransform_vec3(struct Vec3 p, struct Vec3 x_axis,
    struct Vec3 y_axis, struct Vec3 z_axis)
{
    struct Vec3 ret = {0};
    ret.x = x_axis.x*p.x + y_axis.x*p.y + z_axis.x*p.z;
    ret.y = x_axis.y*p.x + y_axis.y*p.y + z_axis.y*p.z;
    ret.z = x_axis.z*p.x + y_axis.z*p.y + z_axis.z*p.z;
    return ret;
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

void *get_os_data(struct GameData *data)
{
	return data->os_data;
}

void  set_os_data(void *os_data, struct GameData *data)
{
	data->os_data = os_data;
}

struct GameData *init(int num_game_states, struct GameState *game_states, void *param, void *os_data, int debug_mode)
{
    struct GameData *data = calloc(1,sizeof(struct GameData));
	set_os_data(os_data,data);
	data->debug_mode = debug_mode;
    data->sprite_shader = load_shader("sprite" SATIN_SHADER_SUFFIX, "sprite" SATIN_SHADER_SUFFIX, data, "pos", "uv",(char*)0);

    data->line_shader = load_shader("line", "line" , data, "pos", (char*)0);

    data->num_game_state_types = num_game_states;
    data->game_state_types = calloc(num_game_states,sizeof(struct GameState));
	memcpy(data->game_state_types, game_states, num_game_states * sizeof(struct GameState));
    
    
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

        /*
        int col_loc=glGetAttribLocation(
            data->shaders[data->line_shader].shader,"vert_col");
        if(col_loc == -1){
            printf("Error: Could not find \"col\" attribute in shader\n");
        }
        glEnableVertexAttribArray(col_loc);
        glVertexAttribPointer(col_loc,4,GL_FLOAT,GL_FALSE,
            6*sizeof(GL_FLOAT),(void*)(2*sizeof(GL_FLOAT)));
         */
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

    
	if (num_game_states > 0) {
		data->active_game_state = 0;
		data->game_states = calloc(1, sizeof(struct GameState));
		data->num_game_states = 1;
		data->game_states[0] = data->game_state_types[0];
		data->game_states[0].init(data, param, -1);

		data->active_game_state = -1;
		/*
		for (int i = 1; i < data->num_game_state_types; i++) {
			struct GameState tmp_game_state = data->game_state_types[i];
			tmp_game_state.init(data, 0, -1);
			tmp_game_state.destroy(data);
		}
		*/
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

struct Vec3 vec3(float x, float y, float z)
{
    struct Vec3 ret = {x,y,z};
    return ret;
}

struct Vec3 normalize_vec3(struct Vec3 v)
{
    float m = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    v.x /= m; v.y /= m; v.z /= m;
    return v;
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

struct Matrix3 multiply_matrix3(struct Matrix3 A, struct Matrix3 B)
{
	struct Matrix3 ret={0};
	for(int i=0;i<3;i++){
	    for(int j=0;j<3;j++){
		for(int k=0;k<3;k++){
		    ret.m[i*3+j]+=A.m[i*3+k]*B.m[k*3+j];
		}
	    }
	}
	return ret;
}

struct Matrix3 get_rotation_matrix3(float x,float y,float z)
{
    float cx = cosf(x);
    float sx = sinf(x);
    float cy = cosf(y);
    float sy = sinf(y);
    float cz = cosf(z);
    float sz = sinf(z);
    struct Matrix3 rot_x =
    {
        1.f,0.f,0.f,
        0.f, cx,-sx,
        0.f, sx, cx,
    };
    struct Matrix3 rot_y =
    {
        cy,0.f, sy,
        0.f,1.f,0.f,
        -sy,0.f, cy,
    };
    struct Matrix3 rot_z =
    {
        cz,-sz,0.f,
        sz, cz,0.f,
        0.f,0.f,1.f,
    };
    struct Matrix3 tmp = multiply_matrix3(rot_x, rot_y);
    return multiply_matrix3(tmp, rot_z);
}

struct Matrix3 lu_decompose_matrix3(struct Matrix3 A)
{
    struct Matrix3 L = {0};
    struct Matrix3 U = {
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
    };
    for(int i=0;i<3;i++){
        M3(L,i,0) = M3(A,i,0);
    }
    float L00 = M3(L,0,0);
    for(int i=1;i<3;i++){
        M3(U,0,i) = M3(A,0,i)/L00;
    }
    
    for(int j=1;j<3-1;j++){
        for(int i=j;i<3;i++){
            float sum = 0.f;
            for(int k=0;k<j;k++){
                sum += M3(L,i,k)*M3(U,k,j);
            }
            M3(L,i,j) = M3(A,i,j) - sum;
        }
        
        for(int k=j+1;k<3;k++){
            float sum = 0.f;
            for(int i=0;i<j;i++){
                sum += M3(L,j,i)*M3(U,i,k);
            }
            float tmp = (M3(A,j,k) - sum);
            if(fabs(tmp) < 1e-15f){
                M3(U, j, k) = 0.f;
            }else{
                float inv_ljj = 1.f/M3(L,j,j);
                M3(U,j,k) = tmp*inv_ljj;
            }
        }
    }
    float sum = 0.f;
    for(int k=0;k<2;k++){
        sum += M3(L,2,k) * M3(U,k,2);
    }
    M3(L,2,2) = M3(A,2,2) - sum;
    
    struct Matrix3 ret = {0};
    for(int c=0;c<3;c++){
        for(int r=0;r<3;r++){
            M3(ret,r,c)  = r < c ? M3(U,r,c) : M3(L,r,c);
        }
    }
    return ret;
}

struct Matrix3 get_translation_matrix3(float x, float y)
{
    struct Matrix3 ret;
    float m[8]=
    {
        1.f, 0.f,0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f
    };
    memcpy(ret.m,m,8*sizeof(float));
	//ret = *(struct Matrix3*)m;
	ret.m[6] = x;
	ret.m[7] = y;
	ret.m[8] = 1.f;
    return ret;
}

struct Matrix3 get_scale_matrix3(float s)
{

    struct Matrix3 ret;
    float m[4]=
    {
        s,0.f,0.f, 0.f
    };
    memcpy(ret.m,m,4*sizeof(float));
    memcpy(ret.m+4,m,4*sizeof(float));
	ret.m[8] = 1.f;
	return ret;
}

struct Matrix3 get_scale_matrix3_non_uniform(float sx, float sy)
{
    struct Matrix3 ret;
    float mx[4]=
    {
        sx,0.f,0.f, 0.f
    };
    float my[4]=
    {
        sy,0.f,0.f, 0.f
    };
    memcpy(ret.m,mx,4*sizeof(float));
    memcpy(ret.m+4,my,4*sizeof(float));
	ret.m[8] = 1.f;
	return ret;
}

struct Matrix3 get_identity_matrix3(void)
{
    struct Matrix3 ret;
    float m[8]=
    {
        1.f,0.f,0.f,
		0.f,1.f,0.f,0.f,
		0.f
    };
    memcpy(ret.m,m,8*sizeof(float));
	ret.m[8] = 1.f;
	return ret;
}

static float det23(struct Matrix3 mat, int i, int j, int k, int l){
    return M3(mat, i,j)*M3(mat,k,l) - M3(mat,i,l)*M3(mat,k,j);
}

struct Matrix3 invert_matrix3(struct Matrix3 mat)
{
    struct Matrix3 ret = {
         det23(mat, 1, 1, 2, 2), -det23(mat, 0, 1, 2, 2), det23(mat, 0, 1, 1, 2),
        -det23(mat, 1, 0, 2, 2), det23(mat, 0, 0, 2, 2), -det23(mat, 0, 0, 1, 2),
         det23(mat, 1, 0, 2, 1), -det23(mat, 0, 0, 2, 1), det23(mat, 0, 0, 1, 1),

    };
    float det = M3(mat,0,0)*det23(mat, 1, 1, 2, 2) - M3(mat,0,1)*det23(mat, 1,0,2,2) + M3(mat,0,2)*det23(mat, 1, 0, 2, 1);
    for(int c=0;c<3;c++){
        for(int r=0;r<3;r++){
            M3(ret,r,c) /= det;
        }
    }
    return ret;
}

struct Matrix3 transpose_matrix3(struct Matrix3 mat)
{
    struct Matrix3 ret = {
        M3(mat,0,0), M3(mat,1,0), M3(mat,2,0),
        M3(mat,0,1), M3(mat,1,1), M3(mat,2,1),
        M3(mat,0,2), M3(mat,1,2), M3(mat,2,2),
    };
    return ret;
}


struct Vec3 solve_LU_matrix3_vec3(struct Matrix3 lu, struct Vec3 y)
{
    struct Vec3 x = {0};

    //Solve Lz = y
    struct Vec3 z = {0};
    for(int i=0;i<3;i++){
        float val = y.m[i];
        for(int j=0;j<i;j++){
            val -= M3(lu,i,j)*z.m[j];
        }
        val /= M3(lu,i,i);
        z.m[i] = val;
    }

    //Solve Ux = z
    for(int i=2;i>=0;i--){
        float val = z.m[i];
        for(int j=(i+1);j<3;j++){
            val -= M3(lu,i,j)*x.m[j];
        }
        x.m[i] = val;
    }

    return x;
}

struct Matrix3 solve_LU_matrix3_matrix3(struct Matrix3 lu, struct Matrix3 Y)
{
    struct Matrix3 ret = {0};
    for(int i_dim = 0; i_dim < 3; i_dim ++){
        struct Vec3 y = {0};
        for(int i=0;i<3;i++){
            y.m[i] = M3(Y,i,i_dim);
        }
        struct Vec3 x = {0};
    
        //Solve Lz = y
        struct Vec3 z = {0};
        for(int i=0;i<3;i++){
            float val = y.m[i];
            for(int j=0;j<i;j++){
                val -= M3(lu,i,j)*z.m[j];
            }
            float old_val = val;
            float div = M3(lu,i,i);
            if(div < 1e-15f){
                val = 0.f;
            }else{
                if(fabsf(val) > 1e-15f){
                    val /= div;
                }
            }
            if(isnan(val) || isinf(val)){
                printf("NAAAAN! %f\n", old_val);
            }
            z.m[i] = val;
        }
    
        //Solve Ux = z
        for(int i=2;i>=0;i--){
            float val = z.m[i];
            for(int j=(i+1);j<3;j++){
                val -= M3(lu,i,j)*x.m[j];
            }
            if(isnan(val)){
                printf("NAAAAN!\n");
            }
            x.m[i] = val;
        }
        for(int i=0;i<3;i++){
            M3(ret,i,i_dim) = x.m[i];
        }
    }
    
    
    return ret;
}

void print_matrix3(struct Matrix3 m)
{
    printf("[\n");
    for(int r=0;r<3;r++){
        for(int c=0;c<3;c++){
            printf("%3.3f ", M3(m,r,c));
        }
        printf(";\n");
    }
    printf("]\n");
}

static const int off_diagonal_i[] = { 0, 0, 1 };
static const int off_diagonal_j[] = { 1, 2, 2 };
static const int off_diagonal_k[] = { 2, 1, 0 };
static const int num_off_diagonal = 3;

void jacobi_diagonalize_matrix3(struct Matrix3 A, struct Matrix3 *eigenvectors,
                                struct Vec3 *eigenvalues, int num_iterations)
{
    struct Matrix3 V = {
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
    };
    struct Matrix3 G = {0};
    for(int i_iteration=0;i_iteration<num_iterations;i_iteration++){
        int i = 0;
        int j = 0;
        int k = 0;
        float Aij = 0.f;
        for(int d=0;d<num_off_diagonal;d++){
            int tmp_i = off_diagonal_i[d];
            int tmp_j = off_diagonal_j[d];
            float val = fabsf(M3(A, tmp_i, tmp_j));
            if(val > Aij){
                Aij = val;
                i = tmp_i;
                j = tmp_j;
                k = off_diagonal_k[d];
            }
        }
        if(Aij < 1e-15f){
            break;
        }
        Aij = M3(A,i,j);
        float Ajj = M3(A,j,j);
        float Aii = M3(A,i,i);
        float theta = atan2f(2.f*Aij, Ajj - Aii)*0.5f;
        float s = sinf(theta);
        float c = cosf(theta);
        memset(G.m, 0, 9*sizeof(float));
        M3(G,k,k) = 1.f;
        M3(G,i,i) = c;
        M3(G,j,j) = c;
        M3(G,i,j) = -s;
        M3(G,j,i) = s;
        //TODO(Vidar):Calculate this directly instead...
        A = multiply_matrix3(G, multiply_matrix3(A, transpose_matrix3(G)));
        V = multiply_matrix3(V, transpose_matrix3(G));
    }
    int indices[3];
    float abs_eigen[3];
    for(int i=0;i<3;i++){
        abs_eigen[i] = fabsf(M3(A,i,i));
        indices[i] = i;
    }
    float tmp[3] = {0};
    int tmp_i[3] = {0};
    merge_sort_auxillary(abs_eigen, tmp, indices, tmp_i, 3);
    for(int i=0;i<3;i++){
        int i2 = indices[i];
        eigenvalues->m[i] = M3(A,i2,i2);
        for(int j=0;j<3;j++){
            M3((*eigenvectors),j,i) = M3(V,j,i2);
        }
    }
}

float det_matrix3(struct Matrix3 A)
{
    float det = M3(A,0,0)*det23(A, 1, 1, 2, 2) - M3(A,0,1)*det23(A, 1,0,2,2) + M3(A,0,2)*det23(A, 1, 0, 2, 1);
    return det;
}

struct Matrix4 matrix3_to_matrix4(struct Matrix3 m)
{
    struct Matrix4 ret ={
        M3(m,0,0), M3(m,0,1), M3(m,0,2), 0.f,
        M3(m,1,0), M3(m,1,1), M3(m,1,2), 0.f,
        M3(m,2,0), M3(m,2,1), M3(m,2,2), 0.f,
        0.f,       0.f,       0.f,       1.0f
    };
    return ret;
}

struct Matrix4 multiply_matrix4(struct Matrix4 a,struct Matrix4 b)
{
    struct Matrix4 ret={0};
    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            for(int k=0;k<4;k++){
                ret.m[i*4+j]+=a.m[i*4+k]*b.m[k*4+j];
            }
        }
    }
    return ret;
}


struct Matrix4 get_trackball_matrix4(float theta, float phi)
{
    struct Matrix4 ret;
    float proj[4][4]=
    {

         {cosf(theta), -sinf(theta)*cosf(phi), sinf(theta)*sinf(phi), 0.0f},
         {0.0f, sinf(phi), cosf(phi), 0.0f},
         {-sinf(theta), -cosf(theta)*cosf(phi), cosf(theta)*sinf(phi), 0.0f},
         {0.0f, 0.0f, 0.0f, 1.0f},
         
        /*
         {cosf(theta),               0.0f,         -sinf(theta),              0.0f},
         {-sinf(theta)*cosf(phi),    sinf(phi),    -cosf(theta)*cosf(phi),    0.0f},
         {sinf(theta)*sinf(phi),     cosf(phi),    cosf(theta)*sinf(phi),     0.0f},
         {0.0f,                      0.0f,         0.0f,                      1.0f},
         
        {cosf(theta), -sinf(theta)*cosf(phi), sinf(theta)*sinf(phi), 0.0f},
        {sinf(theta), cosf(theta)*cosf(phi), -cosf(theta)*sinf(phi), 0.0f},
        {0.0f, sinf(phi), cosf(phi), 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
         */
    };
    memcpy(ret.m,proj,16*sizeof(float));
    return ret;
}

struct Matrix4 get_aspect_correction_matrix4(float w,float h)
{
    float aspect=w/h;
    struct Matrix4 ret={0};
    if(aspect<1.f){
        ret.m[0]=1.f;
        ret.m[5]=aspect;
    } else{
        ret.m[0]=1.f/aspect;
        ret.m[5]=1.f;
    }
    ret.m[10]=1.f;
    ret.m[15]=1.f;
    return ret;
}

struct Matrix4 get_orthographic_matrix4(float scale,float n,float f)
{
    struct Matrix4 ret;
    float proj[4][4]=
    {
        {1.f/scale,0.f,0.f,0.0f},
        {0.f, 1.f/scale, 0.f, 0.0f},
        {0.0f, 0.f, -2.f/(f-n), -(f+n)/(f-n)},
        {0.0f, 0.0f, 0.0f, 1.0f},
    };
    memcpy(ret.m,proj,16*sizeof(float));
    return ret;
}

struct Matrix4 get_perspective_matrix4(float fov,float n,float f)
{
    float top=tanf(fov*0.5f);
    struct Matrix4 ret;
    float proj[4][4]=
    {
        {1.f/top,0.f,0.f,0.0f},
        {0.f, 1.f/top, 0.f, 0.0f},
        {0.0f, 0.f, -(f+n)/(f-n), -1.f},
        {0.0f, 0.0f, -2.f*f*n/(f-n), 0.0f},
    };
    memcpy(ret.m,proj,16*sizeof(float));
    return ret;
}

struct Matrix4 get_translation_matrix4(float x,float y,float z)
{
    struct Matrix4 ret;
    float proj[4][4]=
    {
        {1.f,0.f,0.f, 0.f},
        {0.f, 1.f, 0.f, 0.f},
        {0.0f, 0.f, 1.f, 0.f},
        {x, y, z, 1.0f},
    };
    memcpy(ret.m,proj,16*sizeof(float));
    return ret;
}

struct Matrix4 get_rotation_matrix4(float x,float y,float z)
{
    float cx = cosf(x);
    float sx = sinf(x);
    float cy = cosf(y);
    float sy = sinf(y);
    float cz = cosf(z);
    float sz = sinf(z);
    struct Matrix4 rot_x =
    {
        1.f,0.f,0.f, 0.f,
        0.f, cx,-sx, 0.f,
        0.f, sx, cx, 0.f,
		0.f, 0.f, 0.f, 1.f
    };
    struct Matrix4 rot_y =
    {
        cy,0.f, sy, 0.f,
        0.f,1.f,0.f, 0.f,
        -sy,0.f, cy, 0.f,
		0.f, 0.f, 0.f, 1.f
    };
    struct Matrix4 rot_z =
    {
        cz,-sz,0.f, 0.f,
        sz, cz,0.f, 0.f,
        0.f,0.f,1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
    };
    struct Matrix4 tmp = multiply_matrix4(rot_x, rot_y);
    return multiply_matrix4(tmp, rot_z);
}

struct Matrix4 get_scale_matrix4(float s)
{
    struct Matrix4 ret;
    float proj[4][4]=
    {
        {s  , 0.f,0.f, 0.f},
        {0.f, s  , 0.f, 0.f},
        {0.f, 0.f, s  , 0.f},
        {0.f, 0.f, 0.f, 1.0f},
    };
    memcpy(ret.m,proj,16*sizeof(float));
    return ret;
}

struct Matrix4 get_identity_matrix4(void)
{
    struct Matrix4 ret;
    float proj[4][4] =
    {
        { 1.f, 0.f, 0.f, 0.0f },
        { 0.f, 1.f, 0.f, 0.0f },
        { 0.0f, 0.f, 1.f, 0.f },
        { 0.f , 0.f, 0.f, 1.0f },
    };
    memcpy(ret.m, proj, 16 * sizeof(float));
    return ret;
}

struct Matrix4 transpose_matrix4(struct Matrix4 m)
{
    struct Matrix4 ret = {
        M4(m,0,0), M4(m,1,0), M4(m,2,0), M4(m,3,0),
        M4(m,0,1), M4(m,1,1), M4(m,2,1), M4(m,3,1),
        M4(m,0,2), M4(m,1,2), M4(m,2,2), M4(m,3,2),
        M4(m,0,3), M4(m,1,3), M4(m,2,3), M4(m,3,3),
    };
    return ret;
}

struct Matrix4 invert_matrix4_noscale(struct Matrix4 m)
{
	//NOTE(Vidar): Transpose the rotation and negate the translation
    struct Matrix3 rot = {
         M4(m,0,0), M4(m,0,1), M4(m,0,2),
         M4(m,1,0), M4(m,1,1), M4(m,1,2),
         M4(m,2,0), M4(m,2,1), M4(m,2,2),
    };
	struct Vec3 trans = {
		 -M4(m,3,0), -M4(m,3,1), -M4(m,3,2)
	};
	trans = multiply_matrix3_vec3(rot, trans);
    struct Matrix4 ret = {
        M3(rot,0,0), M3(rot,1,0), M3(rot,2,0), 0.f,
        M3(rot,0,1), M3(rot,1,1), M3(rot,2,1), 0.f,
        M3(rot,0,2), M3(rot,1,2), M3(rot,2,2), 0.f,
		trans.x,     trans.y,     trans.z,     1.f
    };
	return ret;
}

//https://www.gamedev.net/resources/_/technical/math-and-physics/matrix-inversion-using-lu-decomposition-r3637
//TODO(Vidar):Permutation matrix...
struct Matrix4 lu_decompose_matrix4(struct Matrix4 A)
{
    struct Matrix4 L = {0};
    struct Matrix4 U = get_identity_matrix4();
    for(int i=0;i<4;i++){
        M4(L,i,0) = M4(A,i,0);
    }
    float L00 = M4(L,0,0);
    for(int i=1;i<4;i++){
        M4(U,0,i) = M4(A,0,i)/L00;
    }
    
    for(int j=1;j<4-1;j++){
        for(int i=j;i<4;i++){
            float sum = 0.f;
            for(int k=0;k<j;k++){
                sum += M4(L,i,k)*M4(U,k,j);
            }
            M4(L,i,j) = M4(A,i,j) - sum;
        }
        float inv_ljj = 1.f/M4(L,j,j);
        for(int k=j+1;k<4;k++){
            float sum = 0.f;
            for(int i=0;i<j;i++){
                sum += M4(L,j,i)*M4(U,i,k);
            }
            M4(U,j,k) = (M4(A,j,k) - sum)*inv_ljj;
        }
    }
    float sum = 0.f;
    for(int k=0;k<3;k++){
        sum += M4(L,3,k) * M4(U,k,3);
    }
    M4(L,3,3) = M4(A,3,3) - sum;
    
    struct Matrix4 ret = {0};
    for(int c=0;c<4;c++){
        for(int r=0;r<4;r++){
            M4(ret,r,c)  = r < c ? M4(U,r,c) : M4(L,r,c);
        }
    }
    return ret;
}

//Finds x in Ax = y where A = LU
struct Vec4 solve_LU_matrix4_vec4(struct Matrix4 lu, struct Vec4 y)
{
    struct Vec4 x = {0};
    
    //Solve Lz = y
    struct Vec4 z = {0};
    for(int i=0;i<4;i++){
        float val = y.m[i];
        for(int j=0;j<i;j++){
            val -= M4(lu,i,j)*z.m[j];
        }
        val /= M4(lu,i,i);
        z.m[i] = val;
    }
    
    //Solve Ux = z
    for(int i=3;i>=0;i--){
        float val = z.m[i];
        for(int j=(i+1);j<4;j++){
            val -= M4(lu,i,j)*x.m[j];
        }
        x.m[i] = val;
    }
    
    return x;
}

void print_matrix4(struct Matrix4 m)
{
    for(int r=0;r<4;r++){
        printf("[ ");
        for(int c=0;c<4;c++){
            printf("%3.3f ", M4(m,r,c));
        }
        printf("]\n");
    }
}

struct Vec2 add_vec2(struct Vec2 a, struct Vec2 b)
{
    struct Vec2 ret = {a.x + b.x, a.y + b.y};
    return ret;
}

struct Vec2 sub_vec2(struct Vec2 a, struct Vec2 b)
{
    struct Vec2 ret = {a.x - b.x, a.y - b.y};
    return ret;
}

struct Vec2 scale_vec2(float s, struct Vec2 a)
{
    struct Vec2 ret = {a.x*s, a.y*s};
    return ret;
}

float dot_vec2(struct Vec2 a, struct Vec2 b)
{
    return a.x*b.x + a.y*b.y;
}

struct Vec3 multiply_matrix3_vec3(struct Matrix3 m,struct Vec3 v)
{
    struct Vec3 ret = {0};
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            ret.m[i]+=M3(m,i,j)*v.m[j];
        }
    }
    return ret;
}

struct Vec3 multiply_matrix4_vec3(struct Matrix4 m,struct Vec3 v)
{
    struct Vec3 ret = {0};
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            ret.m[i]+=M4(m,i,j)*v.m[j];
        }
    }
    return ret;
}

struct Vec3 multiply_matrix4_vec3_point(struct Matrix4 m,struct Vec3 v)
{
    struct Vec4 v_tmp = {v.x,v.y,v.z,1.f};
    struct Vec4 res = {0};
    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            res.m[i]+=M4(m,i,j)*v_tmp.m[j];
        }
    }
    struct Vec3 ret = {0};
    for(int i=0;i<3;i++){
        ret.m[i] = res.m[i]/res.m[3];
    }
    return ret;
}

struct Vec3 add_vec3(struct Vec3 a, struct Vec3 b)
{
    struct Vec3 ret = {a.x+b.x, a.y+b.y, a.z+b.z};
    return ret;
}

struct Vec3 sub_vec3(struct Vec3 a, struct Vec3 b)
{
    struct Vec3 ret = {a.x-b.x, a.y-b.y, a.z-b.z};
    return ret;
}

struct Vec3 scale_vec3(float s, struct Vec3 a)
{
    struct Vec3 ret = {s*a.x, s*a.y, s*a.z};
    return ret;
}

float dot_vec3(struct Vec3 a, struct Vec3 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

struct Vec3 cross_vec3(struct Vec3 a, struct Vec3 b)
{
    struct Vec3 ret = {
        a.y*b.z-a.z*b.y, -(a.x*b.z-a.z*b.x), a.x*b.y-a.y*b.x
    };
    return ret;
}

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
    struct ShaderUniform *uniforms, struct GameData *data)
{
    int num_textures = 0;
    for(int j=0;j<num_uniforms;j++){
        struct ShaderUniform u = uniforms[j];
        int loc=glGetUniformLocation(shader,u.name);
        if(loc == -1){
            printf("Warning: Could not find \"%s\" uniform in shader\n",
                u.name);
        }else{
            switch(u.type){
                case SHADER_UNIFORM_INT:
                    glUniform1iv(loc, u.num, u.data);
                    break;
                case SHADER_UNIFORM_FLOAT:
                    glUniform1fv(loc, u.num, u.data);
                    break;
                case SHADER_UNIFORM_MAT4:
                    glUniformMatrix4fv(loc,u.num,GL_FALSE,u.data);
                    break;
                case SHADER_UNIFORM_MAT3:
                    glUniformMatrix3fv(loc,u.num,GL_FALSE,u.data);
                    break;
                case SHADER_UNIFORM_VEC4:
                    glUniform4fv(loc,u.num,u.data);
                    break;
                case SHADER_UNIFORM_VEC3:
                    glUniform3fv(loc,u.num,u.data);
                    break;
                case SHADER_UNIFORM_VEC2:
                    glUniform2fv(loc,u.num,u.data);
                    break;
                case SHADER_UNIFORM_TEX2:
                {
                    int texture_id = *(int*)u.data;
                    glUniform1i(loc, num_textures);
                    glActiveTexture(GL_TEXTURE0 + num_textures);
                    glBindTexture(GL_TEXTURE_2D, data->texture_ids[texture_id]);
                    num_textures++;
                    break;
                }
                case SHADER_UNIFORM_SPRITE_POINTER:
                case SHADER_UNIFORM_SPRITE:
                {
                    struct Sprite *sprite;
                    if(u.type == SHADER_UNIFORM_SPRITE_POINTER){
                        sprite = *(struct Sprite**)u.data;
                    }else{
                        int sprite_id = *(int*)u.data;
                        sprite = data->sprites + sprite_id;
                    }
		    /*
		    printf("rendering sprite %f %f %f %f\n",
			    sprite->uv_offset[0], sprite->uv_offset[1],
			    sprite->uv_size[0], sprite->uv_size[1]);
			    */
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
}

void render_meshes(struct FrameData *frame_data, float aspect,
    struct GameData *data)
{
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	int current_depth_test_state = 1;
    struct RenderMeshList *rml = &frame_data->render_mesh_list;
    while(rml != 0){
        for(int i=0;i<rml->num;i++){
            struct RenderMesh *render_mesh = rml->meshes + i;
            struct Mesh *mesh = data->meshes + render_mesh->mesh;
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
        }
        rml = rml->next;
    }
}

void render_quads(struct FrameData *frame_data, float scale, float aspect,
    struct GameData *data)
{
    struct RenderQuadList *list = &frame_data->render_quad_list;
    while(list != 0){
        for(int i=0;i<list->num;i++){
            struct RenderQuad render_quad = list->quads[i];
            int shader = data->shaders[render_quad.shader].shader;
            glUseProgram(shader);
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
        
        glViewport(0, 0, w, h);

        if(frame_data->clear){
            glClearColor(frame_data->clear_color.r, frame_data->clear_color.g,
                         frame_data->clear_color.b, 0.f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        
        render_meshes(frame_data,aspect,data);
        glDisable(GL_DEPTH_TEST);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
        render_quads(frame_data,scale,aspect,data);
    }
}

void render_to_memory(int w, int h, unsigned char *pixels,
    struct FrameData *frame_data, struct GameData *data)
{
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

    GLuint FramebufferName = 0;
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    GLuint renderedTexture;
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
}

void render_to_memory_float(int w, int h, float *pixels,
    struct FrameData *frame_data, struct GameData *data)
{
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


    GLuint FramebufferName = 0;
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    GLuint renderedTexture;
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
}

void render(int framebuffer_w, int framebuffer_h, struct GameData *data)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    render_internal(framebuffer_w, framebuffer_h, data->frame_data, data);
    
    float min_size =
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
}

void update(int ticks, struct InputState input_state, struct GameData *data)
{
    const int num_ticks = sizeof(data->ticks)/sizeof(*data->ticks);
    for(int i=num_ticks-1; i > 0;i--){
        data->ticks[i] = data->ticks[i-1];
    }
    data->ticks[0] = ticks;
    data->active_game_state = data->next_game_state;
    data->game_states[data->active_game_state].update(ticks,input_state,data);
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
}

void end_game(struct GameData *data)
{
    for(int i=0;i<data->num_game_states;i++){
        data->game_states[i].destroy(data);
    }
}
