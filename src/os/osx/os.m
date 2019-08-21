#include "os/os.h"
#include "engine.h"
#include "glut.h"
#include <stdio.h>

#import <Foundation/Foundation.h>

struct OSXData {
    int i;
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
        ofType:extension_string inDirectory:@"data"];
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

static void display_func(void)
{
    if(g_game_data){
        update(0,g_input_state, g_game_data);
        g_input_state.num_keys_typed = 0;
        render(g_framebuffer_w, g_framebuffer_h, g_game_data);
        glutPostRedisplay();
        glutSwapBuffers();
    }else{
        printf("Bad update\n");
    }
}

static void keyboard_func(unsigned char key, int x, int y)
{
    if (g_input_state.num_keys_typed<max_num_keys_typed) {
        g_input_state.keys_typed[g_input_state.num_keys_typed] = key;
        g_input_state.num_keys_typed++;
    }
}

void launch_game(const char *window_title, int _framebuffer_w, int _framebuffer_h, int show_console,
	int num_game_states, void *param, struct GameState *game_states, int debug_mode, void *os_data)
{

    int num_args = 1;
    char *args[1] = {""};
    glutInit(&num_args,args);
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(_framebuffer_w, _framebuffer_h);
	glutCreateWindow(window_title);
    glutDisplayFunc(display_func);
    glutKeyboardFunc(keyboard_func);
    

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

    glutMainLoop();
}

void *os_data_create(void)
{
    return 0;
}
void os_data_set_data_folder_name(void *os_data, char *path)
{
}
char **get_args(int *argc)
{
    return 0;
}
char *get_cwd()
{
    return 0;
}

