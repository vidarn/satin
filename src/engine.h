#pragma once
#include <stdint.h>
#include "linalg/linalg.h"
#include "graphics/graphics.h"
struct GameData;
struct InputState;
struct RenderContext;
struct GameState;
struct FrameData;
struct WindowData;
struct OSData;

struct GameData *init(int num_game_states, struct GameState *game_states, void *param, struct WindowData *window_data, int debug_mode);
int update(int ticks, struct InputState input_state, struct GameData *data);
void render(int framebuffer_w, int framebuffer_h, struct GameData *data);
void end_game(struct GameData *data);
#define TICKS_PER_SECOND 1000000
static const uint64_t ticks_per_second = TICKS_PER_SECOND;
static const float inv_ticks_per_second = 1.f/(float)TICKS_PER_SECOND;

void set_custom_data_pointer(void * custom_data, struct GameData *data);
void *get_custom_data_pointer(struct GameData *data);

void set_common_data_pointer(void * common_data, struct GameData *data);
void *get_common_data_pointer(struct GameData *data);

struct WindowData *get_window_data(struct GameData *data)
;
struct OSData *get_os_data(struct GameData *data)
;

//Graphics
struct Color {
    float r,g,b,a;
};

extern int reference_resolution;


void get_window_extents(float *x_min, float *x_max, float *y_min, float *y_max,
    struct GameData *data);

struct Color rgb(float r, float g, float b);
struct Color rgba(float r, float g, float b, float a);
extern struct Color color_white;
extern struct Color color_black;


void get_sprite_size(int sprite, float *w, float *h, struct GameData *data);
struct Matrix3 get_sprite_matrix(int sprite, struct GameData *data);

void printf_screen(const char *format, int font, float x, float y,
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

void render_mesh(int mesh, int shader, struct Matrix4 mat, struct GraphicsValueSpec *uniforms,
                 int num_uniforms, struct RenderContext *context)
;
void render_mesh_with_callback(int mesh, int shader, struct Matrix4 mat, struct GraphicsValueSpec *uniforms,
	int num_uniforms, void(*callback)(void *param), void *callback_param, struct RenderContext *context)
;

void render_quad(int shader, struct Matrix3 m, struct GraphicsValueSpec *uniforms,
    int num_uniforms, struct RenderContext *context)
;

void render_sprite_screen(int sprite,float x, float y,
    struct RenderContext *context)
;
void render_sprite_screen_scaled(int sprite,float x, float y, float scale,
    struct RenderContext *context)
;
void render_sprite_screen_scaled_with_shader(int sprite,float x, float y,
    float scale, int shader, struct GraphicsValueSpec *uniforms, int num_uniforms,
    struct RenderContext *context)
;

float *get_sprite_uv_offset(int sprite, struct GameData *data)
;
struct Texture *get_sprite_texture(int sprite, struct GameData *data)
;

void set_scissor_state(int enabled, float x1, float y1, float x2, float y2, struct RenderContext* context)
;

void render_to_memory(int w, int h, unsigned char *pixels,
    struct FrameData *frame_data, struct GameData *data);
void render_to_memory_float(int w, int h, float *pixels,
	struct FrameData *frame_data, struct GameData *data);

float get_string_render_width(int font_id, const char *text, int len,
    struct GameData *data);
float get_font_height(int font, struct GameData *data);
int load_font(const char *name, double font_size,
    struct GameData *data);

int load_image_from_memory(int sprite_w, int sprite_h,
	unsigned char* sprite_data, struct GameData* data)
	;
int load_image(const char* name, struct GameData* data)
;
int load_image_from_file(const char* filename, struct GameData* data)
;

int load_sprite_from_memory(int sprite_w, int sprite_h,
    unsigned char *sprite_data, struct GameData *data);
int load_sprite(const char *name, struct GameData *data);
int load_sprite_from_filename(const char *filename, struct GameData *data);
int load_mesh(const char *name, struct GameData *data);
int load_mesh_unit_plane(int shader, struct GameData *data);
int load_mesh_from_memory(int num_verts, struct Vec3 *pos_data,
    struct Vec3 *normal_data, struct Vec2 *uv_data, int num_faces,
    int *face_data, int shader, struct GameData *data);
void update_mesh_from_memory(int mesh_index, int num_verts, struct Vec3 *pos_data,
	struct Vec3 *normal_data, struct Vec2 *uv_data, int num_tris,
	int *tri_data, int shader, struct GameData *data);
void create_sprite_atlas(struct GameData* data)
;

int load_custom_mesh_from_memory(int num_verts, int num_tris,
int *tri_data, int num_data_specs, struct GraphicsValueSpec *data_spec, struct GameData *data)
;

void save_mesh_to_file(int mesh, const char *name, const char *ext, struct GameData *data);
void calculate_mesh_normals(int num_verts, struct Vec3 *pos_data,
    struct Vec3 *normal_data, int num_tris, int *tri_data);
void update_mesh_verts_from_memory(int mesh, struct Vec3 *pos_data,
    struct Vec3 *normal_data, struct Vec2 *uv_data, struct GameData *data);
void update_custom_mesh_verts_from_memory(int mesh, int num_data_specs, struct GraphicsValueSpec *data_spec,
                                          struct GameData *data)
;
int get_mesh_num_verts(int mesh, struct GameData *data);
void get_mesh_vert_data(int mesh, struct Vec3 *pos_data, struct Vec3 *normal_data,
    struct Vec2 *uv_data, struct GameData *data);
int get_mesh_num_tris(int mesh, struct GameData *data);
void get_mesh_tri_data(int mesh, int *tri_data, struct GameData *data);

unsigned int get_mesh_vertex_buffer(int mesh, struct GameData *data);

void update_sprite_from_memory(int sprite, unsigned char *sprite_data,
   struct GameData *data);

void resize_image_from_memory(int sprite, int sprite_w, int sprite_h, unsigned char *sprite_data,
	struct GameData *data)
;

int load_shader(const char* vert_filename, const char * frag_filename, enum GraphicsBlendMode blend_mode, struct GameData *data)
;
int load_shader_from_string(const char* vert_source, const char * frag_source,
	struct GameData *data)
;

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

int cursor_locked(struct GameData *data);
void lock_cursor(struct GameData *data);
void unlock_cursor(struct GameData *data);

int was_key_typed(unsigned int key, struct InputState *input_state);
int is_key_down(int key);

float sum_values(float *values, int num);


//Input
enum MouseState {
    MOUSE_NOTHING,
    MOUSE_CLICKED,
    MOUSE_DOUBLECLICKED,
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
    unsigned int keys_typed[max_num_keys_typed];
	struct ControllerState controllers[max_num_controllers];
};


struct RenderContext
{
    float w,h,offset_x, offset_y;
    struct FrameData *frame_data;
    struct GameData *data;
    struct Matrix4 camera_3d;
    struct Matrix3 camera_2d;
	int disable_depth_test;
    enum GraphicsBlendMode blend_mode;
};


struct GameState {
    int (*update)(int ticks, struct InputState input_state,
        struct GameData *data);
    void (*init)(struct GameData *data, void *argument, int parent_state);
    void (*destroy)(struct GameData *data);
    void *custom_data;
    int should_destroy;
    //TODO(Vidar): Add activate, deactivate...
};


