#pragma once
#include <stdint.h>
struct GameData;
struct InputState;
struct RenderContext;
struct GameState;
struct FrameData;
struct Vec2{
    float x,y;
};


// A 3x3 matrix look like this:
// [ 0  1  2]
// [ 3  4  5]
// [ 6  7  8]
struct Matrix3{
    float m[9];
};
#define M3(mat,x,y) mat.m[x*3+y]
struct Vec3{
    union{
        float m[3];
        struct{
            float x,y,z;
        };
    };
};
struct Matrix4{
    float m[16];
};
#define M4(mat,x,y) mat.m[x*4+y]
struct Vec4{
    union{
        float m[4];
        struct{
        float x,y,z,w;
        };
    };
};

struct GameData *init(int num_game_states, struct GameState *game_states, void *param);
void update(int ticks, struct InputState input_state, struct GameData *data);
void render(int framebuffer_w, int framebuffer_h, struct GameData *data);
void end_game(struct GameData *data);
#define TICKS_PER_SECOND 1000000
static const uint64_t ticks_per_second = TICKS_PER_SECOND;
static const float inv_ticks_per_second = 1.f/(float)TICKS_PER_SECOND;

void set_custom_data_pointer(void * custom_data, struct GameData *data);
void *get_custom_data_pointer(struct GameData *data);

void set_common_data_pointer(void * common_data, struct GameData *data);
void *get_common_data_pointer(struct GameData *data);

//Graphics
struct Color {
    float r,g,b,a;
};

extern int reference_resolution;

struct Shader;
#define SHADER_UNIFORM_TYPES \
SHADER_UNIFORM_TYPE(INT,   sizeof(int))\
SHADER_UNIFORM_TYPE(FLOAT,   sizeof(float))\
SHADER_UNIFORM_TYPE(MAT4,  16*sizeof(float))\
SHADER_UNIFORM_TYPE(MAT3,   9*sizeof(float))\
SHADER_UNIFORM_TYPE(VEC4,   4*sizeof(float))\
SHADER_UNIFORM_TYPE(VEC3,   3*sizeof(float))\
SHADER_UNIFORM_TYPE(VEC2,   2*sizeof(float))\
SHADER_UNIFORM_TYPE(TEX2,   sizeof(int))\
SHADER_UNIFORM_TYPE(SPRITE, sizeof(int))\
SHADER_UNIFORM_TYPE(SPRITE_POINTER, sizeof(void *))

enum ShaderUniformType{
#define SHADER_UNIFORM_TYPE(name,size) SHADER_UNIFORM_##name,
    SHADER_UNIFORM_TYPES
#undef SHADER_UNIFORM_TYPE
};
struct ShaderUniform{
    char *name;
    enum ShaderUniformType type;
    int num;
    void *data;
};

void get_window_extents(float *x_min, float *x_max, float *y_min, float *y_max,
    struct GameData *data);

struct Vec3 vec3(float x, float y, float z);
struct Vec3 normalize_vec3(struct Vec3);

struct Color rgb(float r, float g, float b);
struct Color rgba(float r, float g, float b, float a);

struct Matrix3 multiply_matrix3(struct Matrix3 a, struct Matrix3 b);
struct Matrix3 get_rotation_matrix3(float x,float y,float z);
struct Matrix3 lu_decompose_matrix3(struct Matrix3 mat);
struct Matrix3 invert_matrix3(struct Matrix3 mat);
struct Matrix3 transpose_matrix3(struct Matrix3 mat);
struct Vec3 solve_LU_matrix3_vec3(struct Matrix3 lu, struct Vec3 y);
struct Matrix3 solve_LU_matrix3_matrix3(struct Matrix3 lu, struct Matrix3 y);
void print_matrix3(struct Matrix3);
void jacobi_diagonalize_matrix3(struct Matrix3 A, struct Matrix3 *eigenvectors,
    struct Vec3 *eigenvalues, int num_iterations);
float det_matrix3(struct Matrix3 A);

struct Matrix4 matrix3_to_matrix4(struct Matrix3);

struct Matrix4 multiply_matrix4(struct Matrix4 a,struct Matrix4 b);
struct Matrix4 get_trackball_matrix4(float theta, float phi);
struct Matrix4 get_aspect_correction_matrix4(float w,float h);
struct Matrix4 get_perspective_matrix4(float fov,float n,float f);
struct Matrix4 get_orthographic_matrix4(float scale,float n,float f);
struct Matrix4 get_translation_matrix4(float x,float y,float z);
struct Matrix4 get_scale_matrix4(float s);
struct Matrix4 get_identity_matrix4(void);
struct Matrix4 transpose_matrix4(struct Matrix4);
struct Matrix4 lu_decompose_matrix4(struct Matrix4 mat);
struct Vec4 solve_LU_matrix4_vec4(struct Matrix4 lu, struct Vec4 y);
void print_matrix4(struct Matrix4);

struct Vec2 add_vec2(struct Vec2 a, struct Vec2 b);
struct Vec2 sub_vec2(struct Vec2 a, struct Vec2 b);
struct Vec2 scale_vec2(float s, struct Vec2 a);
float dot_vec2(struct Vec2 a, struct Vec2 b);

struct Vec3 multiply_matrix3_vec3(struct Matrix3 m,struct Vec3 v);
struct Vec3 multiply_matrix4_vec3(struct Matrix4 m,struct Vec3 v);
struct Vec3 multiply_matrix4_vec3_point(struct Matrix4 m,struct Vec3 v);
struct Vec3 add_vec3(struct Vec3 a, struct Vec3 b);
struct Vec3 sub_vec3(struct Vec3 a, struct Vec3 b);
struct Vec3 scale_vec3(float s, struct Vec3 a);
float dot_vec3(struct Vec3 a, struct Vec3 b);
struct Vec3 cross_vec3(struct Vec3 a, struct Vec3 b);

void get_sprite_size(int sprite, float *w, float *h, struct GameData *data);
struct Matrix3 get_sprite_matrix(int sprite, struct GameData *data);

void printf_screen(char *format, int font, float x, float y,
    struct Color color, struct RenderContext *context, ...);
void render_string_screen_n(const char *string, int n, int font, float *x,
    float *y, struct Color color, struct RenderContext *context);
void render_string_screen(const char *string, int font, float *x, float *y,
    struct Color color, struct RenderContext *context);
void render_string_screen_fancy(const char *string, int font, float *x, float *y, float anchor_y, float width, struct Color color,
    struct RenderContext *context);

void render_line_screen(float x1, float y1, float x2, float y2, float thickness,
    struct Color color, struct RenderContext *context);
void render_rect_screen(float x1, float y1, float x2, float y2, float thickness,
    struct Color color, struct RenderContext *context);

void render_mesh(int mesh, struct Matrix4 mat, struct ShaderUniform *uniforms,
                 int num_uniforms, struct RenderContext *context);

void render_quad(int shader, struct Matrix3 m, struct ShaderUniform *uniforms,
    int num_uniforms, struct RenderContext *context);

void render_sprite_screen(int sprite,float x, float y,
    struct RenderContext *context);
void render_sprite_screen_scaled(int sprite,float x, float y, float scale,
    struct RenderContext *context);

void render_to_memory(int w, int h, unsigned char *pixels,
    struct FrameData *frame_data, struct GameData *data);

float get_string_render_width(int font_id, const char *text, int len,
    struct GameData *data);
float get_font_height(int font, struct GameData *data);
int load_font(const char *name, double font_size,
    struct GameData *data);

int load_sprite_from_memory(int sprite_w, int sprite_h,
    unsigned char *sprite_data, struct GameData *data);
int load_sprite(const char *name, struct GameData *data);
int load_mesh(const char *name, int shader,
    struct GameData *data);
int load_mesh_from_memory(int num_verts, struct Vec3 *pos_data,
    struct Vec3 *normal_data, struct Vec2 *uv_data, int num_faces,
    int *face_data, int shader, struct GameData *data);
void calculate_mesh_normals(int num_verts, struct Vec3 *pos_data,
    struct Vec3 *normal_data, int num_tris, int *tri_data);
void update_mesh_verts_from_memory(int mesh, struct Vec3 *pos_data,
    struct Vec3 *normal_data, struct Vec2 *uv_data, struct GameData *data);
int get_mesh_num_verts(int mesh, struct GameData *data);
void get_mesh_vert_data(int mesh, struct Vec3 *pos_data, struct Vec3 *normal_data,
    struct Vec2 *uv_data, struct GameData *data);
int get_mesh_num_tris(int mesh, struct GameData *data);
void get_mesh_tri_data(int mesh, int *tri_data, struct GameData *data);

void update_sprite_from_memory(int sprite, unsigned char *sprite_data,
   struct GameData *data);

int load_shader(const char* vert_filename,const char * frag_filename,
    struct GameData *data, ...);

int get_mean_ticks(int length, struct GameData *data);
int get_tick_length(struct GameData *data);
int get_ticks(int i, struct GameData *data);

struct FrameData *frame_data_new(void);
void frame_data_reset(struct FrameData *frame_data);
void frame_data_clear(struct FrameData *frame_data, struct Color col);
void set_active_frame_data(struct FrameData *frame_data, struct GameData *data);
struct FrameData *get_active_frame_data(struct GameData *data);

int add_game_state(int state_type, struct GameData *data, void *argument);
void remove_game_state(int state, struct GameData *data);
void switch_game_state(int state, struct GameData *data);
int get_active_game_state(struct GameData *data);

struct Vec2 transform_vector(struct Vec2 p, struct Vec2 x_axis, struct Vec2 y_axis);
struct Vec2 untransform_vector(struct Vec2 p, struct Vec2 x_axis, struct Vec2 y_axis);

struct Vec3 transform_vec3(struct Vec3 p, struct Vec3 x_axis, struct Vec3 y_axis,
    struct Vec3 z_axis);
struct Vec3 untransform_vec3(struct Vec3 p, struct Vec3 x_axis, struct Vec3 y_axis,
    struct Vec3 z_axis);


float sum_values(float *values, int num);

//Input
enum MouseState {
    MOUSE_NOTHING,
    MOUSE_CLICKED,
    MOUSE_DRAG,
    MOUSE_DRAG_RELEASE,
};

enum ControllerButtonMask {
	CONTROLLER_BUTTON_DPAD_UP = 1<<0,
	CONTROLLER_BUTTON_DPAD_DOWN = 1<<1,
	CONTROLLER_BUTTON_DPAD_LEFT = 1<<2,
	CONTROLLER_BUTTON_DPAD_RIGHT = 1<<3,
	CONTROLLER_BUTTON_START = 1<<4,
	CONTROLLER_BUTTON_BACK = 1<<5,
	CONTROLLER_BUTTON_LEFT_THUMB = 1<<6,
	CONTROLLER_BUTTON_RIGHT_THUMB = 1<<7,
	CONTROLLER_BUTTON_LEFT_SHOULDER = 1<<8,
	CONTROLLER_BUTTON_RIGHT_SHOULDER = 1<<9,
	CONTROLLER_BUTTON_A = 1<<10,
	CONTROLLER_BUTTON_B = 1<<11,
	CONTROLLER_BUTTON_X = 1<<12,
	CONTROLLER_BUTTON_Y = 1<<13,
};
extern float controller_left_thumb_deadzone;
extern float controller_right_thumb_deadzone;
struct ControllerState
{
	int valid;
	uint64_t buttons_pressed;
	float left_trigger;
	float right_trigger;
	float left_thumb[2];
	float right_thumb[2];
};

#define max_num_keys_typed 16
#define max_num_controllers 4

struct InputState
{
    float mouse_x, mouse_y;
    float delta_x, delta_y;
    float drag_start_x, drag_start_y;
    float scroll_delta_x, scroll_delta_y;
    int prev_mouse_state, prev_mouse_state_middle, prev_mouse_state_right;
    int mouse_state, mouse_state_middle, mouse_state_right;
    int mouse_down, mouse_down_middle, mouse_down_right;
    int num_keys_typed;
    int keys_typed[max_num_keys_typed];
	struct ControllerState controllers[max_num_controllers];
};


struct RenderContext
{
    float w,h,offset_x, offset_y;
    struct FrameData *frame_data;
    struct GameData *data;
    struct Matrix4 camera_3d;
};


struct GameState {
    void (*update)(int ticks, struct InputState input_state,
        struct GameData *data);
    void (*init)(struct GameData *data, void *argument, int parent_state);
    void (*destroy)(struct GameData *data);
    void *custom_data;
    int should_destroy;
    //TODO(Vidar): Add activate, deactivate...
};


