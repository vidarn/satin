#include "os/os.h"
#include "engine.h"
#include <stdlib.h>

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


void launch_game(const char *window_title, int _framebuffer_w, int _framebuffer_h, int show_console, 
	int num_game_states, void *param, struct GameState *game_states, int debug_mode)
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
