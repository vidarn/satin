
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

/*
struct Shader *window_compile_shader(const char *vert_filename,
        const char *frag_filename, enum GraphicsBlendMode blend_mode,
        char *error_buffer, int error_buffer_len,
        struct WindowData *window_data)
;
*/
