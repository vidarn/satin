#include "os/os.h"
#include "engine.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

char *get_file_path(const char *filename, const char *extension)
{
    return 0;
}

FILE *open_file(const char *filename, const char *extension, const char *mode, struct OSData *os_data)
{
    size_t len = 5 + strlen(filename) + 1 + strlen(extension) + 1;
    char *path = calloc(len,1);
    sprintf(path,"data/%s.%s",filename,extension);
    printf("Trying to open %s\n", path);
    FILE *fp = fopen(path,mode);
    free(path);
    if (!fp) {
        printf("File open failed\n");
    }
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


struct OSData *os_data_create(void)
{
    return 0;
}

void os_data_set_data_folder_name(struct OSData *os_data, char *path)
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
