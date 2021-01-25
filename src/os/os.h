#pragma once
#include <stdio.h>
#include <inttypes.h>

struct GameData;
struct OSData;

char *get_file_path(const char *filename, const char *extension);
FILE *open_file(const char *filename, const char *extension, const char *mode, struct OSData *os_data);
char *get_save_file_name(const char *title);
char *get_open_file_name(const char *title);
size_t get_file_len(FILE *fp);
int os_get_num_cores(void)
;
const char *get_computer_name(void);

void os_set_clipboard_contents(void* os_data, char* string, size_t len)
;

extern char os_folder_separator;
void os_path_strip_leaf(char *path);
int os_is_path_valid(char *path);

int atomic_increment_int32(int *a);


struct OSData *os_data_create(void);
void os_data_set_data_folder_name(struct OSData *os_data, char *path);
void save_screenshot(const char* filename)
;

char **get_args(int *argc);
char *get_cwd(void);

enum OS_LIST_ENTRIES_TYPE {
	OS_LIST_ENTRIES_TYPE_FOLDER = 1,
	OS_LIST_ENTRIES_TYPE_FILE = 2,
};
//NOTE(Vidar): You have to free the strings returned in entries
int os_list_entries_in_folder(const char *path, const char **entries, int max_num_entries, enum OS_LIST_ENTRIES_TYPE type)
;
int os_list_resource_entries(const char *data_path, const char** entries, int max_num_entries, enum OS_LIST_ENTRIES_TYPE type, struct OSData *os_data)
;
int os_does_file_exist(const char *filename)
;
int os_does_folder_exist(const char *path)
;
int os_make_folder(const char* path)
;
