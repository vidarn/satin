#include "os.h"
#import <Foundation/Foundation.h>

FILE *open_file(const char *filename, const char *extension, const char *mode)
{
    NSBundle *main_bundle = [NSBundle mainBundle];
    NSString *filename_string = [NSString stringWithUTF8String:filename];
    NSString *extension_string = [NSString stringWithUTF8String:extension];
    NSString *resourcePath = [main_bundle pathForResource:filename_string
        ofType:extension_string inDirectory:@"data"];
    FILE *fp = fopen([resourcePath cStringUsingEncoding:NSUTF8StringEncoding],
                                                                    mode);
    return fp;
}
