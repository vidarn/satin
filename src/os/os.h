#pragma once
#include <stdio.h>
#include <inttypes.h>


char *get_file_path(const char *filename, const char *extension);
FILE *open_file(const char *filename, const char *extension, const char *mode);
char *get_save_file_name(const char *title);
char *get_open_file_name(const char *title);
uint64_t get_current_tick(void);
size_t get_file_len(FILE *fp);
int get_num_cores(void);
const char *get_computer_name(void);

void launch_game(const char *window_title, int _framebuffer_w, int _framebuffer_h, int show_console,
	int num_game_states, void *param, struct GameState *game_states);
