/* v1.2.0 - add column display (default mode)
 * Uses ioctl(TIOCGWINSZ) to detect terminal width
 * and print files in evenly spaced columns.
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>

/*----------------------------------------------------------
 * Function: print_permissions
 * Purpose: show file permissions in rwx format
 *----------------------------------------------------------*/
void print_permissions(mode_t mode) {
    putchar(S_ISDIR(mode) ? 'd' : '-');
    putchar(mode & S_IRUSR ? 'r' : '-');
    putchar(mode & S_IWUSR ? 'w' : '-');
    putchar(mode & S_IXUSR ? 'x' : '-');
    putchar(mode & S_IRGRP ? 'r' : '-');
    putchar(mode & S_IWGRP ? 'w' : '-');
    putchar(mode & S_IXGRP ? 'x' : '-');
    putchar(mode & S_IROTH ? 'r' : '-');
    putchar(mode & S_IWOTH ? 'w' : '-');
    putchar(mode & S_IXOTH ? 'x' : '-');
}

/*----------------------------------------------------------
 * list_dir_long(): same as v1.1.0
 *----------------------------------------------------------*/
void list_dir_long(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return;
    }

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;

        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", path, de->d_name);

        struct stat st;
        if (stat(full, &st) == -1) {
            perror("stat");
            continue;
        }

        print_permissions(st.st_mode);
        printf(" %3ld", (long)st.st_nlink);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        printf(" %-8s %-8s", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");

        printf(" %8ld", (long)st.st_size);

        char *time_str = ctime(&st.st_mtime);
        time_str[strlen(time_str)-1] = '\0';
        printf(" %s %s\n", time_str, de->d_name);
    }
    closedir(d);
}

/*----------------------------------------------------------
 * list_dir_columns(): print file names in columns
 *----------------------------------------------------------*/
void list_dir_columns(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return;
    }

    // Step 1: Read all names into an array
    char **names = NULL;
    size_t count = 0;
    size_t maxlen = 0;
    struct dirent *de;

    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;

        names = realloc(names, (count + 1) * sizeof(char *));
        names[count] = strdup(de->d_name);

        size_t len = strlen(de->d_name);
        if (len > maxlen) maxlen = len;
        count++;
    }
    closedir(d);

    if (count == 0) return;

    // Step 2: Get terminal width using ioctl()
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col ? w.ws_col : 80;

    // Step 3: Calculate column width & number of columns
    int col_width = (int)maxlen + 2;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;

    // Step 4: Print in columns (top-to-bottom, then left-to-right)
    int rows = (count + cols - 1) / cols;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c * rows + r;
            if (idx < (int)count)
                printf("%-*s", col_width, names[idx]);
        }
        printf("\n");
    }

    // Step 5: Free memory
    for (size_t i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

/*----------------------------------------------------------
 * main(): decides whether to use -l or columns
 *----------------------------------------------------------*/
int main(int argc, char *argv[]) {
    int long_format = 0;
    int i = 1;

    if (argc > 1 && strcmp(argv[1], "-l") == 0) {
        long_format = 1;
        i = 2;
    }

    if (i >= argc) {
        if (long_format) list_dir_long(".");
        else list_dir_columns(".");
    } else {
        for (; i < argc; ++i) {
            printf("%s:\n", argv[i]);
            if (long_format) list_dir_long(argv[i]);
            else list_dir_columns(argv[i]);
            if (i + 1 < argc) printf("\n");
        }
    }
    return 0;
}
