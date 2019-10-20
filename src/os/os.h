#pragma once
#include <stdio.h>
#include <inttypes.h>

struct GameData;
struct GameState;

char *get_file_path(const char *filename, const char *extension);
FILE *open_file(const char *filename, const char *extension, const char *mode, struct GameData *data);
char *get_save_file_name(const char *title);
char *get_open_file_name(const char *title);
uint64_t get_current_tick(void);
size_t get_file_len(FILE *fp);
int get_num_cores(void);
const char *get_computer_name(void);
int os_is_key_down(int key);

extern char os_folder_separator;
void os_path_strip_leaf(char *path);
int os_is_path_valid(char *path);

int atomic_increment_int32(int *a);

void launch_game(const char *window_title, int _framebuffer_w, int _framebuffer_h, int show_console,
	int num_game_states, void *param, struct GameState *game_states, int debug_mode, void *os_data)
;

void *os_data_create(void);
void os_data_set_data_folder_name(void *os_data, char *path);
struct GraphicsData;
struct GraphicsData *os_data_get_graphics(void *os_data);
char **get_args(int *argc);
char *get_cwd(void);

enum OS_LIST_ENTRIES_TYPE {
	OS_LIST_ENTRIES_TYPE_FOLDER = 1,
	OS_LIST_ENTRIES_TYPE_FILE = 2,
};
int os_list_entries_in_folder(const char *path, const char **entries, int max_num_entries, enum OS_LIST_ENTRIES_TYPE type)
;
int os_does_file_exist(const char *filename)
;
