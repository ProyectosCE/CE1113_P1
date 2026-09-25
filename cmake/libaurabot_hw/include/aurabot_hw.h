#ifndef AURABOT_HW_H
#define AURABOT_HW_H

#include "aurabot_hw_types.h"
#include "aurabot_motor.h"
#include "aurabot_sensor.h"
#include "aurabot_led.h"
#include "aurabot_audio.h"


aurabot_hw_status_t aurabot_hw_init(void);

aurabot_hw_status_t aurabot_hw_shutdown(void);


#endif