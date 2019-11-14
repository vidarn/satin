#include "glut.h"
#include "engine.h"
#include <stdio.h>

struct WindowData{
    struct OSData *os_data;
    struct GraphicsData *graphics_data;
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

static void special_key_func(int key, int x, int y)
{
    if (g_input_state.num_keys_typed<max_num_keys_typed) {
        g_input_state.keys_typed[g_input_state.num_keys_typed] = key + 255;
        g_input_state.num_keys_typed++;
    }
}

void launch_game(const char *window_title, int _framebuffer_w, int _framebuffer_h, int show_console,
    int num_game_states, void *param, struct GameState *game_states, int debug_mode, struct OSData *os_data)
{
    
    int num_args = 1;
    char *args[1] = {""};
    glutInit(&num_args,args);
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(_framebuffer_w, _framebuffer_h);
	glutCreateWindow(window_title);
    glutDisplayFunc(display_func);
    glutKeyboardFunc(keyboard_func);
    glutSpecialFunc(special_key_func);

	if (!os_data) {
		os_data = os_data_create();
	}
    struct WindowData *window_data = calloc(1,sizeof(struct WindowData));
    window_data->os_data = os_data;
    
    g_game_data = 0;
    struct GraphicsData *graphics = graphics_create(0);
    window_data->graphics_data = graphics;

	struct GameData *game_data = init(num_game_states, game_states, param, os_data, graphics, debug_mode);
    g_game_data = game_data;
    struct InputState input_state= {0};
    g_input_state = input_state;
    g_framebuffer_w = _framebuffer_w;
    g_framebuffer_h = _framebuffer_h;

    glutMainLoop();
}

struct OSData *window_get_os_data(struct WindowData *window)
{
	return data->os_data;
}

struct Shader *window_compile_shader(const char *vert_filename,
        const char *frag_filename, enum GraphicsBlendMode blend_mode,
        char *error_buffer, int error_buffer_len,
        struct WindowData *window_data)
{
    struct OSData *os_data = window_data->os_data;
    struct GraphicsData *graphics_data = window_data->graphics_data;
    struct Shader *shader = 0;
    FILE *vert_fp = open_file(vert_filename, "glsl", "rt", os_data);
    if(vert_fp){
        FILE *frag_fp = open_file(frag_filename, "glsl", "rt", os_data);
        if(frag_fp){
            fseek(vert_fp, 0, SEEK_END);
            fseek(frag_fp, 0, SEEK_END);
            size_t vert_len = ftell(vert_fp);
            size_t frag_len = ftell(vert_fp);
            rewind(vert_fp);
            rewind(frag_fp);
            char *vert_buffer = calloc(vert_len + frag_len, 1);
            char *frag_buffer = vert_buffer + vert_len;
            fread(vert_buffer, vert_len, 1, vert_fp);
            fread(frag_buffer, frag_len, 1, frag_fp);
            shader = graphics_compile_shader(frag_buffer, vert_buffer,
                    blend_mode, error_buffer, error_buffer_len, graphics_data);
            free(vert_buffer);
        }
        fclose(vert_fp);
    }
    return shader;
}
