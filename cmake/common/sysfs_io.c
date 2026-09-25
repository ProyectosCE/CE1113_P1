#include <errno.h>
#include <stdio.h>

#include "sysfs_io.h"

/* Write a text value to a sysfs file. */
int sysfs_write_text(const char *path, const char *text)
{
    if (path == NULL || text == NULL) {
        errno = EINVAL;
        return -1;
    }

    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return -1;
    }

    if (fputs(text, file) == EOF) {
        int saved_errno = errno;
        (void)fclose(file);
        errno = saved_errno;
        return -1;
    }

    return fclose(file) == 0 ? 0 : -1;
}

/* Read one integer from a sysfs file. */
int sysfs_read_int(const char *path, int *value)
{
    if (path == NULL || value == NULL) {
        errno = EINVAL;
        return -1;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return -1;
    }

    if (fscanf(file, "%d", value) != 1) {
        (void)fclose(file);
        errno = EIO;
        return -1;
    }

    return fclose(file) == 0 ? 0 : -1;
}