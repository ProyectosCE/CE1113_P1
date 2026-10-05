#ifndef AURABOT_TIME_UTILS_H
#define AURABOT_TIME_UTILS_H

#include <time.h>

long long time_elapsed_ms(const struct timespec *start,
                          const struct timespec *end);

#endif
