/* v1.1.0 - add long listing (-l)
 * Demonstrates stat(), getpwuid(), getgrgid(), and ctime().
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

/*----------------------------------------------------------
 * print_permissions(): prints the rwx bits for a file
 *----------------------------------------------------------*/
void print_permissions(mode_t mode) {
    putchar(S_ISDIR(mode) ? 'd' : '-');          // directory?
    putchar(mode & S_IRUSR ? 'r' : '-');         // owner read
    putchar(mode & S_IWUSR ? 'w' : '-');         // owner write
    putchar(mode & S_IXUSR ? 'x' : '-');         // owner exec
    putchar(mode & S_IRGRP ? 'r' : '-');         // group read
    putchar(mode & S_IWGRP ? 'w' : '-');         // group write
    putchar(mode & S_IXGRP ? 'x' : '-');         // group exec
    putchar(mode & S_IROTH ? 'r' : '-');         // others read
    putchar(mode & S_IWOTH ? 'w' : '-');         // others write
    putchar(mode & S_IXOTH ? 'x' : '-');         // others exec
}

/*----------------------------------------------------------
 * list_dir_long(): implements 'ls -l'
 *----------------------------------------------------------*/
void list_dir_long(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return;
    }

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;  // skip hidden files

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
        time_str[strlen(time_str)-1] = '\0';   // remove newline
        printf(" %s %s\n", time_str, de->d_name);
    }
    closedir(d);
}

/*----------------------------------------------------------
 * list_dir_simple(): original single-column ls
 *----------------------------------------------------------*/
void list_dir_simple(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return;
    }
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        printf("%s\n", de->d_name);
    }
    closedir(d);
}

/*----------------------------------------------------------
 * main(): parse arguments and call the right function
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
        else list_dir_simple(".");
    } else {
        for (; i < argc; ++i) {
            printf("%s:\n", argv[i]);
            if (long_format) list_dir_long(argv[i]);
            else list_dir_simple(argv[i]);
            if (i + 1 < argc) printf("\n");
        }
    }
    return 0;
}
