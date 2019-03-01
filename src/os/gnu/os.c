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

#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>

FILE *open_file(const char *filename,const char *extension,const char *mode, struct GameData *data)
{
	//NOTE(Vidar):We assume 1024 chars is enough
	char exe_path[1024];
	ssize_t read_bytes = readlink("/proc/self/exe", exe_path, 1024);
	if(read_bytes<0){
		printf("Error, could not read /proc/self/exe\n");
		return 0;
	}

	exe_path[read_bytes] = 0;
	char *base_path = dirname(exe_path);

	//const char *base_path = "/home/pi/code/material_scanner_server/scanner_app/deps/satin/data/";
	size_t len = strlen(base_path) + strlen(filename) + strlen(extension) + 32;
	char *path = calloc(len,1);
	sprintf(path,"%s/data/%s%s",base_path,filename,extension);
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
	//XGrabPointer(display, root_window, 0, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
	XSelectInput(display, window, ButtonPressMask);

	GLXContext glx_context = glXCreateContext(display, visual_info, 0 , GL_TRUE);
	/*
	int context_attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		None
	};
	GLXContext glx_context =
		glXCreateContextAttribsARB(display, visual_info, 0, GL_TRUE,
		context_attribs);
	*/
	glXMakeCurrent(display, window, glx_context);

	opengl_load();

	//glXSwapIntervalMESA(1);

	struct GNUData* os_data = os_data_create();
	struct GameData* game_data = init(num_game_states, game_states, param, &os_data, debug_mode);


	int wait_for_event = 0;
	while(1){
		struct InputState input_state = {0};
		while(XPending(display) > 0 || wait_for_event){
			wait_for_event = 0;
			XEvent event;
			XNextEvent(display , &event);
			
			switch(event.type){
			case Expose:
			{
				XWindowAttributes window_attributes;
				XGetWindowAttributes(display, window, &window_attributes);
				glViewport(0, 0, window_attributes.width, window_attributes.height);
				break;
			}
				
			case KeyPress:
			{
				glXMakeCurrent(display, None, NULL);
				glXDestroyContext(display, glx_context);
				XDestroyWindow(display, window);
				XCloseDisplay(display);
				exit(0);
				break;
			}
			case ButtonPress:
			{

				XButtonEvent button_event = *(XButtonEvent*)&event;
				float p_x = (float)button_event.x;
				float p_y = (float)button_event.y;
				float m_x,m_y;

				float w = (float)framebuffer_w;
				float h = (float)framebuffer_h;
				if (w<h) {
					m_x = p_x / w;
					m_y = 1.f - (p_y / w - 0.5*(h - w) / w);
				}
				else {
					m_x = p_x / h - 0.5f*(w - h) / h;
					m_y = 1.f - p_y / h;
				}
				input_state.mouse_x = m_x;
				input_state.mouse_y = m_y;

				input_state.mouse_state = MOUSE_CLICKED;
				printf("Button press! %f %f\n", input_state.mouse_x, input_state.mouse_y);
				break;
			}
			}
		}
		//TODO(Vidar): Report correct delta time
		wait_for_event = update(0, input_state, game_data);
		render(framebuffer_w, framebuffer_h, game_data);
		glXSwapBuffers(display, window);
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
