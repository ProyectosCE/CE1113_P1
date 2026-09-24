#ifndef AURABOT_HW_H
#define AURABOT_HW_H


typedef enum {
    AURABOT_HW_OK = 0,
    AURABOT_HW_ERROR = -1,
    AURABOT_HW_INVALID_ARGUMENT = -2,
    AURABOT_HW_NOT_INITIALIZED = -3,
    AURABOT_HW_NOT_AVAILABLE = -4,
    AURABOT_HW_NOT_IMPLEMENTED = -5
} aurabot_hw_status_t;


typedef enum {
    AURABOT_MOTOR_LEFT = 0,
    AURABOT_MOTOR_RIGHT
} aurabot_motor_id_t;


typedef enum {
    AURABOT_PROXIMITY_FRONT = 0,
    AURABOT_PROXIMITY_SIDE
} aurabot_proximity_sensor_id_t;


typedef enum {
    AURABOT_LED_SYSTEM = 0,
    AURABOT_LED_AUTONOMOUS,
    AURABOT_LED_MANUAL,
    AURABOT_LED_OBSTACLE
} aurabot_led_id_t;


aurabot_hw_status_t aurabot_hw_init(void);

aurabot_hw_status_t aurabot_hw_shutdown(void);


/*
 * Temporary motor interface.
 * It will be replaced by the public motor API 
 */
struct Motor {
    int Pin1;
    int Pin2;
    int pwm;
};

int SetMotor(struct Motor *m);

int ApagarMotor(struct Motor *m);

int SetMoDir(struct Motor *m, char dir);

int SetMoPwm(struct Motor *motor, int duty_percent);


#endif