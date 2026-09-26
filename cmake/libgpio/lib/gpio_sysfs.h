#ifndef GPIO_SYSFS_H
#define GPIO_SYSFS_H

#ifndef GPIO_SYSFS_PATH
#define GPIO_SYSFS_PATH "/sys/class/gpio"
#endif

int gpio_sysfs_number(int pin);
int gpio_sysfs_configure(int pin, const char *mode);
int gpio_sysfs_release(int pin);

#endif