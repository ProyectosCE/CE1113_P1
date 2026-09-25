#ifndef SYSFS_IO_H
#define SYSFS_IO_H

int sysfs_write_text(const char *path, const char *text);
int sysfs_read_int(const char *path, int *value);

#endif