/* v1.0.0 - basic ls: list non-hidden entries, one per line */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

void list_dir_simple(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return;
    }
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue; // skip hidden files
        printf("%s\n", de->d_name);
    }
    closedir(d);
}

int main(int argc, char *argv[]) {
    if (argc == 1)
        list_dir_simple(".");
    else {
        for (int i = 1; i < argc; ++i) {
            list_dir_simple(argv[i]);
            if (i + 1 < argc)
                printf("\n");
        }
    }
    return 0;
}
