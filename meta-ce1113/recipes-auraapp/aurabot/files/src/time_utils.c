#include "time_utils.h"

long long time_elapsed_ms(const struct timespec *start,
                          const struct timespec *end)
{
    return (long long)(end->tv_sec - start->tv_sec) * 1000LL +
           (end->tv_nsec - start->tv_nsec) / 1000000LL;
}
