#include "os/os.h"
#include "engine.h"
#include <GL/glew.h>
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "os/win32/key_codes.h"
#include <emscripten.h>
#include <emscripten/html5.h>

static int last_tick = 0;
static char g_keyboard_state[512] = {0};

struct EMSData {
    int i;
};

char *get_file_path(const char *filename, const char *extension)
{
    return 0;
}
FILE *open_file(const char *filename, const char *extension, const char *mode, struct GameData *data)
{
    size_t len = strlen(filename) + strlen(extension) + 1;
    char *path = calloc(len,1);
    sprintf(path,"%s%s",filename,extension);
    FILE *fp = fopen(path,mode);
    free(path);
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
    return last_tick;
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
    return g_keyboard_state[key];
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
      double width, height;
      //emscripten_get_canvas_size(&w, &h, &fs);
      emscripten_get_element_css_size("#canvas", &width, &height);
    g_framebuffer_w = (int)width;
    g_framebuffer_h = (int)height;
    int tick = glutGet(GLUT_ELAPSED_TIME) * (TICKS_PER_SECOND/1000);
    int delta_ticks = tick - last_tick;
    last_tick = tick;
    if(g_game_data){
        update(delta_ticks,g_input_state, g_game_data);
        g_input_state.num_keys_typed = 0;
        render(g_framebuffer_w, g_framebuffer_h, g_game_data);
        glutPostRedisplay();
        glutSwapBuffers();
    }else{
        printf("Bad update\n");
    }
}

static void keyboard_up_func(unsigned char key, int x, int y)
{
    g_keyboard_state[key > 96 ? key - 32 : key] = 0;
}

static void keyboard_func(unsigned char key, int x, int y)
{
    g_keyboard_state[key > 96 ? key - 32 : key] = 1;
    if (g_input_state.num_keys_typed<max_num_keys_typed) {
        g_input_state.keys_typed[g_input_state.num_keys_typed] = key;
        g_input_state.num_keys_typed++;
    }
}

static void special_key_up_func(int key, int x, int y)
{
    g_keyboard_state[key + 255] = 0;
}

static void special_key_func(int key, int x, int y)
{
    g_keyboard_state[key + 255] = 1;
    if (g_input_state.num_keys_typed<max_num_keys_typed) {
        g_input_state.keys_typed[g_input_state.num_keys_typed] = key + 255;
        g_input_state.num_keys_typed++;
    }
}

void reshape_func(int w, int h)
{
    g_framebuffer_w = w;
    g_framebuffer_h = h;
}

EM_BOOL on_canvassize_changed(int eventType, const void *reserved, void *userData)
{
  int w, h, fs;
  emscripten_get_canvas_size(&w, &h, &fs);
  double cssW, cssH;
  emscripten_get_element_css_size(0, &cssW, &cssH);
  printf("Canvas resized: WebGL RTT size: %dx%d, canvas CSS size: %02gx%02g\n", w, h, cssW, cssH);
    g_framebuffer_w = w;
    g_framebuffer_h = h;
  return 0;
}


void launch_game(const char *window_title, int _framebuffer_w, int _framebuffer_h, int show_console,
	int num_game_states, void *param, struct GameState *game_states, int debug_mode, void *os_data)
{
     printf("ccc\n");

    /*
    EmscriptenFullscreenStrategy s;
  memset(&s, 0, sizeof(s));
  s.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_ASPECT;
  s.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
  s.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
  s.canvasResizedCallback = on_canvassize_changed;
  EMSCRIPTEN_RESULT ret = emscripten_enter_soft_fullscreen("#canvas", &s);
  */

      //int w, h, fs;
      double width, height;
      //emscripten_get_canvas_size(&w, &h, &fs);
      emscripten_get_element_css_size("#canvas", &width, &height);
    //emscripten_set_canvas_size((int)width, (int)height);

    int num_args = 1;
    char *args[1] = {""};
    glutInit(&num_args,args);
	//glutInitWindowSize(_framebuffer_w,_framebuffer_h);
	//glutInitWindowSize(w,h);
	glutInitWindowSize((int)width, (int)height);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutCreateWindow(window_title);
    glutDisplayFunc(display_func);
    glutKeyboardFunc(keyboard_func);
    glutKeyboardUpFunc(keyboard_up_func);
    glutSpecialFunc(special_key_func);
    glutSpecialUpFunc(special_key_up_func);
    glutReshapeFunc(reshape_func);
    GLenum err = glewInit();
    if (GLEW_OK != err){
        printf("Glew init error\n");
    }

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

    //glutFullScreen();


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
