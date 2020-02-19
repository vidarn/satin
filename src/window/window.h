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

struct OSData *window_get_os_data(struct WindowData *window)
;

struct GraphicsData *window_get_graphics_data(struct WindowData *window)
;

uint64_t window_get_current_tick(struct WindowData *window_data)
;

void window_get_extents(float *x_min, float *x_max, float *y_min, float *y_max,
    struct WindowData *window)
;
