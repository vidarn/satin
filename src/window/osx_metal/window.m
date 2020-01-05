
#include "engine.h"
#include "os/os.h"

#import <MetalKit/MetalKit.h>

static struct InputState g_input_state;
static struct GameData *g_game_data;

int satin_num_game_states;
void *satin_param;
struct GameState *satin_game_states;

//NOTE(Vidar):Ugly way of getting the MTKView from swift to objective-c
MTKView *satin_mtk_view = 0;

struct WindowData{
    struct OSData *os_data;
    struct GraphicsData *graphics_data;
};

struct OSData *window_get_os_data(struct WindowData *window)
{
    return window->os_data;
}

struct GraphicsData *window_get_graphics_data(struct WindowData *window)
{
    return window->graphics_data;
}

void launch_game(const char *window_title, int _framebuffer_w, int _framebuffer_h, int show_console,
    int num_game_states, void *param, struct GameState *game_states, int debug_mode, void *os_data)
{
    satin_num_game_states = num_game_states;
    satin_game_states = game_states;
    
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    [satin_mtk_view setDevice:device];
    [satin_mtk_view setColorPixelFormat:MTLPixelFormatBGRA8Unorm];
    
    
    g_game_data = 0;

    if (!os_data) {
        os_data = os_data_create();
    }
    
    struct GraphicsData *graphics = graphics_create((__bridge void *)(satin_mtk_view));
    struct WindowData *window_data = calloc(1,sizeof(struct WindowData));
    window_data->graphics_data = graphics;
    window_data->os_data = os_data;
    
    struct GameData *game_data = init(num_game_states, game_states, param, window_data, debug_mode);
    g_game_data = game_data;
    struct InputState input_state= {0};
    g_input_state = input_state;    
}

void satin_update(struct InputState input_state)
{
    if(g_game_data){
        update(0,input_state, g_game_data);
        g_input_state.num_keys_typed = 0;
        float w = satin_mtk_view.drawableSize.width;
        float h = satin_mtk_view.drawableSize.height;
        render((int)w, (int)h, g_game_data);
    }else{
        printf("Bad update\n");
    }
}
