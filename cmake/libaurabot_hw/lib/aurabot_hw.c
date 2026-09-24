#include "../include/aurabot_hw.h"

#include "aurabot_hw_internal.h"


static int hw_initialized = 0;


aurabot_hw_status_t aurabot_hw_init(void)
{
    hw_initialized = 1;

    return AURABOT_HW_OK;
}


aurabot_hw_status_t aurabot_hw_shutdown(void)
{
    hw_initialized = 0;

    return AURABOT_HW_OK;
}


int aurabot_hw_is_initialized(void)
{
    return hw_initialized;
}