#include "image.h"
#include <sys/stat.h>
#include <stdio.h>

int image_check(const char *image) {
    char path[256];
    struct stat st;
    snprintf(path, sizeof(path), "images/%s", image);
    if (stat(path, &st) == 0) return 0; 
    printf("Error: Image '%s' not found.\n", image);
    return 1; 
}
