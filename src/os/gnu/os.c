#include "os/os.h"
#include "engine.h"
#include <stdlib.h>
#include <string.h>

struct GNUData {
    //WCHAR data_base_path[_SATIN_OPEN_FILE_MAX_PATH_LEN];
};

void *os_data_create(void)
{
	struct GNUData *os_data = calloc(1, sizeof(struct GNUData));
	return os_data;
}
void os_data_set_data_folder_name(void *os_data, char *path)
{
}

FILE *open_file(const char *filename,const char *extension,const char *mode, struct GameData *data)
{
	const char *base_path = "/home/pi/code/material_scanner_server/scanner_app/deps/satin/data/";
	size_t len = strlen(base_path) + strlen(filename) + strlen(extension) + 16;
	char *path = calloc(len,1);
	sprintf(path,"%s%s%s",base_path,filename,extension);
	printf("Opening file %s\n", path);
	FILE *fp = fopen(path, mode);
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
	return 0;
}

const char *get_computer_name(void)
{
	return 0;
}

int get_num_cores(void)
{
	return 1;
}

int os_is_key_down(int key) {
	return 0;
}

#include "opengl.h"
#include <stdio.h>
#include <math.h>

int atomic_increment_int32(int *a) {
	return 0;
}


#include<X11/X.h>
#include<X11/Xlib.h>
#include<GL/glx.h>

void launch_game(const char *window_title, int framebuffer_w, int framebuffer_h, int show_console, 
	int num_game_states, void *param, struct GameState *game_states, int debug_mode)
{
	struct InputState input_state = {0};
	Display *display = XOpenDisplay(0);
	if(!display){
		printf("Error! Could not connect to X server\n");
		return;
	}
	Window root_window = DefaultRootWindow(display);
	GLint visual_attributes[] = {
		GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER,
		None //End of list sentinel
	};
	XVisualInfo *visual_info = glXChooseVisual(display, 0, visual_attributes);
	if(!visual_info){
		printf("Error! Not matching visual found\n");
		return;
	}
	printf("Found visual info\n");
	Colormap colormap = XCreateColormap(display, root_window,
		visual_info->visual, AllocNone);
	printf("Created colormap\n");
	XSetWindowAttributes window_attributes = {0};
	window_attributes.colormap = colormap;
	window_attributes.event_mask = ExposureMask | KeyPressMask;
	Window window = XCreateWindow(display, root_window, 0, 0,
		framebuffer_w, framebuffer_h, 0, visual_info->depth,
		InputOutput, visual_info->visual,
		CWColormap|CWEventMask, &window_attributes);
	printf("Created window\n");
	XMapWindow(display, window);
	XStoreName(display, window, window_title);

	GLXContext glx_context = glXCreateContext(display, visual_info, 0 , GL_TRUE);
	glXMakeCurrent(display, window, glx_context);

	opengl_load();

	struct GNUData* os_data = os_data_create();
	struct GameData* game_data = init(num_game_states, game_states, param, &os_data, debug_mode);


	while(1){
		XEvent event;
		XNextEvent(display , &event);
		
		if(event.type == Expose) {
			XWindowAttributes window_attributes;
			XGetWindowAttributes(display, window, &window_attributes);
			glViewport(0, 0, window_attributes.width, window_attributes.height);
			//TODO(Vidar): Report correct delta time
			update(0, input_state, game_data);
			render(framebuffer_w, framebuffer_h, game_data);
			glXSwapBuffers(display, window);
		}
			
		else if(event.type == KeyPress) {
			glXMakeCurrent(display, None, NULL);
			glXDestroyContext(display, glx_context);
			XDestroyWindow(display, window);
			XCloseDisplay(display);
			exit(0);
		}
	}
	end_game(game_data);

}

char **get_args(int *argc)
{
	return 0;
}

char *get_cwd()
{
	return 0;
}
