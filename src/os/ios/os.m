#include "engine.h"
#include "renderer.h"
#include <stdio.h>

#import <Foundation/Foundation.h>
#import <MetalKit/MetalKit.h>

//NOTE(Vidar):Ugly way of getting the MTKView from swift to objective-c
MTKView *satin_mtk_view = 0;

struct OSData {
    struct GraphicsData *graphics;
};

char *get_file_path(const char *filename, const char *extension)
{
    return 0;
}
FILE *open_file(const char *filename, const char *extension, const char *mode, struct GameData *data)
{
    NSBundle *main_bundle = [NSBundle mainBundle];
    if(!main_bundle){
        printf("Could not load bundle\n");
    }
    NSString *filename_string = [NSString stringWithUTF8String:filename];
    NSString *extension_string = [NSString stringWithUTF8String:extension];
    NSString *resourcePath = [main_bundle pathForResource:filename_string
        ofType:extension_string];
    FILE *fp = fopen([resourcePath cStringUsingEncoding:NSUTF8StringEncoding], mode);
    return fp;
}


char *get_save_file_name(const char *title)
{
    return 0;
}
char *get_open_file_name(const char *title)
{
    return 0;
}
uint64_t get_current_tick(void)
{
    return 0;
}
size_t get_file_len(FILE *fp)
{
    return 0;
}
int get_num_cores(void)
{
    return 0;
}
const char *get_computer_name(void)
{
    return 0;
}
int os_is_key_down(int key)
{
    return 0;
}

int atomic_increment_int32(int *a)
{
    return 0;
}

static struct InputState g_input_state;
static struct GameData *g_game_data;
static int g_framebuffer_w;
static int g_framebuffer_h;

int satin_num_game_states;
void *satin_param;
struct GameState *satin_game_states;

void *os_data_create(void)
{
    struct OSData *os_data = calloc(1,sizeof(struct OSData));
    os_data->graphics = graphics_create((__bridge void *)(satin_mtk_view));
    return os_data;
}
void os_data_set_data_folder_name(void *os_data, char *path)
{
}
struct GraphicsData *os_data_get_graphics(void *os_data)
{
    return ((struct OSData *)os_data)->graphics;
}
char **get_args(int *argc)
{
    return 0;
}
char *get_cwd()
{
    return 0;
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
    struct GameData *game_data = init(num_game_states, game_states, param, os_data, debug_mode);
    g_game_data = game_data;
    struct InputState input_state= {0};
    g_input_state = input_state;
    g_framebuffer_w = _framebuffer_w;
    g_framebuffer_h = _framebuffer_h;
    
}

void satin_update(void)
{
    if(g_game_data){
        update(0,g_input_state, g_game_data);
        g_input_state.num_keys_typed = 0;
        render(g_framebuffer_w, g_framebuffer_h, g_game_data);
    }else{
        printf("Bad update\n");
    }
}
