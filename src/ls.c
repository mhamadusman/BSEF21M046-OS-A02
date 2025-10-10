/* v1.5.0 - Colorized Output using ANSI Escape Codes
 * Adds color based on file type:
 *  - Blue: Directories
 *  - Green: Executables
 *  - Red: Archives (.zip, .tar, .gz)
 *  - Magenta: Symbolic Links
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
 * ANSI Color Codes
 *----------------------------------------------------------*/
#define RESET   "\033[0m"
#define BLUE    "\033[1;34m"
#define GREEN   "\033[1;32m"
#define RED     "\033[1;31m"
#define MAGENTA "\033[1;35m"

/*----------------------------------------------------------
 * Function to choose color based on file type
 *----------------------------------------------------------*/
const char* get_color(const char *name, mode_t mode) {
    if (S_ISDIR(mode)) return BLUE;
    if (S_ISLNK(mode)) return MAGENTA;
    if (S_ISREG(mode) && (mode & S_IXUSR)) return GREEN; // executable

    // check for archive extensions
    const char *ext = strrchr(name, '.');
    if (ext && (!strcmp(ext, ".tar") || !strcmp(ext, ".gz") || !strcmp(ext, ".zip")))
        return RED;

    return RESET;
}

/*----------------------------------------------------------
 * Comparator for alphabetical sorting
 *----------------------------------------------------------*/
int compare_names(const void *a, const void *b) {
    const char *s1 = *(const char **)a;
    const char *s2 = *(const char **)b;
    return strcmp(s1, s2);
}

/*----------------------------------------------------------
 * Print rwx permissions for -l
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
 * list_dir_long() - (-l)
 *----------------------------------------------------------*/
void list_dir_long(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return;
    }

    char **names = NULL;
    size_t count = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        names = realloc(names, (count + 1) * sizeof(char *));
        names[count++] = strdup(de->d_name);
    }
    closedir(d);

    qsort(names, count, sizeof(char *), compare_names);

    for (size_t i = 0; i < count; i++) {
        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", path, names[i]);

        struct stat st;
        if (lstat(full, &st) == -1) {
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

        const char *color = get_color(names[i], st.st_mode);
        printf(" %s %s%s%s\n", time_str, color, names[i], RESET);

        free(names[i]);
    }
    free(names);
}

/*----------------------------------------------------------
 * list_dir_columns() - default (vertical)
 *----------------------------------------------------------*/
void list_dir_columns(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return;
    }

    char **names = NULL;
    size_t count = 0, maxlen = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        names = realloc(names, (count + 1) * sizeof(char *));
        names[count++] = strdup(de->d_name);
        size_t len = strlen(de->d_name);
        if (len > maxlen) maxlen = len;
    }
    closedir(d);

    qsort(names, count, sizeof(char *), compare_names);

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col ? w.ws_col : 80;
    int col_width = (int)maxlen + 2;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;

    int rows = (count + cols - 1) / cols;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c * rows + r;
            if (idx < (int)count) {
                struct stat st;
                char full[1024];
                snprintf(full, sizeof(full), "%s/%s", path, names[idx]);
                if (lstat(full, &st) == -1) continue;
                const char *color = get_color(names[idx], st.st_mode);
                printf("%s%-*s%s", color, col_width, names[idx], RESET);
            }
        }
        printf("\n");
    }

    for (size_t i = 0; i < count; i++) free(names[i]);
    free(names);
}

/*----------------------------------------------------------
 * list_dir_horizontal() - (-x)
 *----------------------------------------------------------*/
void list_dir_horizontal(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return;
    }

    char **names = NULL;
    size_t count = 0, maxlen = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        names = realloc(names, (count + 1) * sizeof(char *));
        names[count++] = strdup(de->d_name);
        size_t len = strlen(de->d_name);
        if (len > maxlen) maxlen = len;
    }
    closedir(d);

    qsort(names, count, sizeof(char *), compare_names);

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col ? w.ws_col : 80;
    int col_width = (int)maxlen + 2;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;

    int rows = (count + cols - 1) / cols;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = r * cols + c;
            if (idx < (int)count) {
                struct stat st;
                char full[1024];
                snprintf(full, sizeof(full), "%s/%s", path, names[idx]);
                if (lstat(full, &st) == -1) continue;
                const char *color = get_color(names[idx], st.st_mode);
                printf("%s%-*s%s", color, col_width, names[idx], RESET);
            }
        }
        printf("\n");
    }

    for (size_t i = 0; i < count; i++) free(names[i]);
    free(names);
}

/*----------------------------------------------------------
 * main() - parse options and run display functions
 *----------------------------------------------------------*/
int main(int argc, char *argv[]) {
    int long_format = 0;
    int horizontal = 0;
    int i = 1;

    while (i < argc && argv[i][0] == '-') {
        if (strcmp(argv[i], "-l") == 0) long_format = 1;
        else if (strcmp(argv[i], "-x") == 0) horizontal = 1;
        else break;
        i++;
    }

    if (i >= argc) {
        if (long_format) list_dir_long(".");
        else if (horizontal) list_dir_horizontal(".");
        else list_dir_columns(".");
    } else {
        for (; i < argc; ++i) {
            printf("%s:\n", argv[i]);
            if (long_format) list_dir_long(argv[i]);
            else if (horizontal) list_dir_horizontal(argv[i]);
            else list_dir_columns(argv[i]);
            if (i + 1 < argc) printf("\n");
        }
    }
    return 0;
}
