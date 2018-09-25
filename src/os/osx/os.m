#include "os.h"
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

static NSString * get_file_path_internal(const char *filename,
    const char *extension)
{
    NSString *filename_string = [NSString stringWithUTF8String:filename];
    NSString *extension_string = [NSString stringWithUTF8String:extension];
#if 0
    NSBundle *main_bundle = [NSBundle mainBundle];
    NSString *resourcePath = [main_bundle pathForResource:filename_string
                                                   ofType:extension_string inDirectory:@"data"];
#else
    NSString *base_folder= @"/Users/vidarn/Dropbox/skola/masters_thesis/code/data/";
    NSString *resourcePath = [[base_folder stringByAppendingString:filename_string] stringByAppendingString:extension_string];
#endif
    return resourcePath;
}

FILE *open_file(const char *filename, const char *extension, const char *mode)
{
        NSString *path = get_file_path_internal(filename, extension);
        FILE *fp = fopen([path cStringUsingEncoding:NSUTF8StringEncoding],
         mode);
    return fp;
}

char* get_file_path(const char *filename, const char *extension)
{
    
    NSString *path = get_file_path_internal(filename, extension);
    char *ret = strdup([path cStringUsingEncoding:NSUTF8StringEncoding]);
    return ret;
}

char *get_save_file_name(const char *title)
{
    NSSavePanel *save_panel = [NSSavePanel savePanel];
    NSString *title_string = [NSString stringWithUTF8String:title];
    //[save_panel setAllowedFileTypes:@[[NSString stringWithUTF8String:"tikz" ], [NSString stringWithUTF8String:"png" ]]];
    [save_panel setTitle:title_string];
    //[save_panel runModal];
    [save_panel performSelectorOnMainThread:@selector(runModal) withObject:nil waitUntilDone:YES];
    //NSURL *url = [save_panel URL];
    //NSString *url_string = [url path];
    //const char *c = [url_string UTF8String];
    const char *c = "AAA";
    //[save_panel close];
    return strdup(c);
}


char *get_open_file_name(const char *title)
{
    NSOpenPanel *open_panel = [NSOpenPanel openPanel];
    NSString *title_string = [NSString stringWithUTF8String:title];
    [open_panel setAllowedFileTypes:@[[NSString stringWithUTF8String:"png"]]];
    [open_panel setTitle:title_string];
    [open_panel runModal];
    NSURL *url = [open_panel URL];
    NSString *url_string = [url path];
    const char *c = [url_string UTF8String];
    [open_panel close];
    return strdup(c);
}

#include <mach/mach_time.h>
#include "engine.h"
static const uint64_t NANOS_PER_USEC = 1000ULL;
static const uint64_t NANOS_PER_MILLISEC = 1000ULL * NANOS_PER_USEC;
static const uint64_t NANOS_PER_SEC = 1000ULL * NANOS_PER_MILLISEC;
uint64_t get_current_tick(void)
{
    return mach_absolute_time() * ticks_per_second / NANOS_PER_SEC;
}

size_t get_file_len(FILE *fp)
{
#ifdef _MSC_VER
    _fseeki64(fp,0,SEEK_END);
    size_t f_len = _ftelli64(fp);
#else
    fseek(fp,0,SEEK_END);
    size_t f_len = ftell(fp);
#endif
    rewind(fp);
    return f_len;
}

#include <sys/types.h>
#include <sys/sysctl.h>
int get_num_cores(void)
{
    int count;
    size_t count_len = sizeof(count);
    sysctlbyname("hw.logicalcpu", &count, &count_len, NULL, 0);
    return count;
}
