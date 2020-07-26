#include "engine.h"
#include "renderer/renderer.h"
#include <stdio.h>

#import <Foundation/Foundation.h>

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
    NSString *path_string = [@"data/" stringByAppendingString:filename_string];
    NSString *extension_string = [NSString stringWithUTF8String:extension];
    NSString *resourcePath = [main_bundle pathForResource:path_string
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


int atomic_increment_int32(int *a)
{
    return 0;
}

void *os_data_create(void)
{
    struct OSData *os_data = calloc(1,sizeof(struct OSData));
    return os_data;
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

