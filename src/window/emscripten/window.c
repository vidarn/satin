#include "engine.h"
#include "os/os.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include "os/win32/key_codes.h"

static struct InputState g_input_state = {0};
static struct GameData *g_game_data;
static char g_keyboard_state[512] = {0};
static double last_time_ms = 0;
static int mouse_down;
static int touch_down;

static char* canvas_name = "#canvas";

struct WindowData {
	struct OSData* os_data;
	struct GraphicsData* graphics_data;
	float min_x, max_x, min_y, max_y;
    int framebuffer_w;
	int framebuffer_h;
    float pixel_ratio;
};

static struct WindowData* g_window_data;

static void get_canvas_res(float* w, float* h)
{
    float pixel_ratio = emscripten_get_device_pixel_ratio();
    g_window_data->pixel_ratio = pixel_ratio;
    double width, height;
    emscripten_get_element_css_size(canvas_name, &width, &height);
    *w = width  * pixel_ratio;
    *h = height * pixel_ratio;
}


struct OSData *window_get_os_data(struct WindowData *window)
{
	return window->os_data;
}

struct GraphicsData *window_get_graphics_data(struct WindowData *window)
{
    return window->graphics_data;
}

void window_get_extents(float *x_min, float *x_max, float *y_min, float *y_max,
    struct WindowData *window)
{
    *x_min = window->min_x;
    *x_max = window->max_x;
    *y_min = window->min_y;
    *y_max = window->max_y;
}

void window_get_res(float* w, float* h, struct WindowData* window)
{
	*w = (float)window->framebuffer_w;
	*h = (float)window->framebuffer_h;
}

int window_is_key_down(int key)
{
    /*
    switch(key){
        case KEY_LEFT:
            return g_keyboard_state[255 + GLUT_KEY_LEFT];
            break;
        case KEY_RIGHT:
            return g_keyboard_state[255 + GLUT_KEY_RIGHT];
            break;
        case KEY_UP:
            return g_keyboard_state[255 + GLUT_KEY_UP];
            break;
        case KEY_DOWN:
            return g_keyboard_state[255 + GLUT_KEY_DOWN];
            break;
    }
    */
    return g_keyboard_state[key];
}

EM_BOOL update_callback(double time, void* userData)
{
    struct GameData* game_data = userData;
    int wait_for_event = update(0, g_input_state, game_data);
    return EM_TRUE;
}

EM_BOOL resize_callback(int eventType, const EmscriptenUiEvent* uiEvent, void* userData)
{
    float width, height;
    get_canvas_res(&width, &height);
    emscripten_set_canvas_element_size(canvas_name, width, height);
    g_window_data->framebuffer_w = (int)width;
    g_window_data->framebuffer_h = (int)height;
    return EM_TRUE;
}

EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData)
{
    g_input_state.mouse_p.x = mouseEvent->targetX*g_window_data->pixel_ratio;
    g_input_state.mouse_p.y = g_window_data->framebuffer_h - mouseEvent->targetY*g_window_data->pixel_ratio;
    mouse_down = mouseEvent->buttons & 1;
    return EM_TRUE;
}

EM_BOOL touch_callback(int eventType, const EmscriptenTouchEvent* touchEvent, void* userData)
{
    EmscriptenTouchPoint point = touchEvent->touches[0];
    g_input_state.mouse_p.x = point.targetX*g_window_data->pixel_ratio;
    g_input_state.mouse_p.y = g_window_data->framebuffer_h - point.targetY*g_window_data->pixel_ratio;
    if (eventType == EMSCRIPTEN_EVENT_TOUCHSTART) {
        touch_down = 1;
    }
    return EM_TRUE;
}

void main_loop_callback()
{
    double time_ms = emscripten_get_now();
    if (last_time_ms == 0.0) {
        last_time_ms = time_ms;
    }
    double dt = time_ms - last_time_ms;
    int ticks = (int)(((double)TICKS_PER_SECOND * dt) / 1000.f);
    last_time_ms = time_ms;

    int framebuffer_w = g_window_data->framebuffer_w;
    int framebuffer_h = g_window_data->framebuffer_h;
    float min_size =
        (float)(framebuffer_w > framebuffer_h ? framebuffer_h : framebuffer_w);
    float pad = fabsf((float)(framebuffer_w - framebuffer_h))*0.5f/min_size;
    g_window_data->min_x = 0.f; g_window_data->max_x = 1.f;
    g_window_data->min_y = 0.f; g_window_data->max_y = 1.f;
    if(framebuffer_w > framebuffer_h){
        g_window_data->min_x -= pad;
        g_window_data->max_x += pad;
    }else{
        g_window_data->min_y -= pad;
        g_window_data->max_y += pad;
    }

    g_input_state.mouse_down = touch_down || mouse_down;
    touch_down = 0;

    update(ticks, g_input_state, g_game_data);
    render(framebuffer_w, framebuffer_h, g_game_data);
}


void launch_game(const char *window_title, int _framebuffer_w, int _framebuffer_h, int show_console,
	int num_game_states, void *param, struct GameState *game_states, int debug_mode, void *os_data)
{

    emscripten_set_window_title(window_title);

    EmscriptenWebGLContextAttributes attributes = {0};
    emscripten_webgl_init_context_attributes(&attributes);
    attributes.majorVersion = 2;
    attributes.minorVersion = 0;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context_handle = emscripten_webgl_create_context(canvas_name, &attributes);
    assert(context_handle > 0);
    EMSCRIPTEN_RESULT res = emscripten_webgl_make_context_current(context_handle);
    assert(res == EMSCRIPTEN_RESULT_SUCCESS);
    assert(emscripten_webgl_get_current_context() == context_handle);

	struct WindowData* window_data = calloc(1, sizeof(struct WindowData));
    g_window_data = window_data;

    float width, height;
    get_canvas_res(&width, &height);
    emscripten_set_canvas_element_size(canvas_name, width, height);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_FALSE, resize_callback);

    emscripten_set_mousemove_callback(canvas_name, 0, EM_FALSE, mouse_callback);
    emscripten_set_mousedown_callback(canvas_name, 0, EM_FALSE, mouse_callback);
    emscripten_set_mouseup_callback(canvas_name, 0, EM_FALSE, mouse_callback);

    //emscripten_set_touchmove_callback(canvas_name, 0, EM_FALSE, touch_callback);
    emscripten_set_touchstart_callback(canvas_name, 0, EM_FALSE, touch_callback);
    emscripten_set_touchend_callback(canvas_name, 0, EM_FALSE, touch_callback);
    emscripten_set_touchmove_callback(canvas_name, 0, EM_FALSE, touch_callback);
    emscripten_set_touchcancel_callback(canvas_name, 0, EM_FALSE, touch_callback);


    window_data->framebuffer_w = (int)width;
    window_data->framebuffer_h = (int)height;

	if (!os_data) {
		os_data = os_data_create();
	}
    window_data->os_data = os_data;
    struct GraphicsData *graphics = graphics_create(0);
    window_data->graphics_data = graphics;

	struct GameData* game_data = init(num_game_states, game_states, param, window_data, debug_mode);
    g_game_data = game_data;

    emscripten_set_main_loop(main_loop_callback, 0, 0);
    emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);

}
