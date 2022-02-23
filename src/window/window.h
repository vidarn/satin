#pragma once
#include <stdint.h>
struct WindowData;
struct GameState;
struct OSData;

struct GraphicsData;
struct Shader;
enum GraphicsBlendMode;

void launch_game(const char *window_title, int _framebuffer_w, int _framebuffer_h, int show_console,
    int num_game_states, void *param, struct GameState *game_states, int debug_mode, struct OSData *os_data)
;
struct WindowStartGameParams {
    const char *window_title;
    int _framebuffer_w;
    int _framebuffer_h;
    int show_console;
    int num_game_states;
    void* param;
    struct GameState* game_states;
    int debug_mode;
    struct OSData* os_data;
    struct GameData* game_data; //You shouldn't fill this in yourself. This parameter is used by the live reload system
};
struct GameData* window_start_game(struct WindowStartGameParams params)
;
int window_step_game(struct GameData* game_data)
;
void window_end_game(struct GameData* game_data)
;
struct OSData *window_get_os_data(struct WindowData *window)
;
struct GraphicsData *window_get_graphics_data(struct WindowData *window)
;
uint64_t window_get_current_tick(struct WindowData *window_data)
;
void window_get_extents(float *x_min, float *x_max, float *y_min, float *y_max,
    struct WindowData *window)
;
void window_get_res(float *w, float *h, struct WindowData *window)
;
int window_is_key_down(int key)
;
