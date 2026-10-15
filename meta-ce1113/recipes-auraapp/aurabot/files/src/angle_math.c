#include "angle_math.h"
#include "aurabot_config.h"

/* Valores escalados por AURABOT_TRIG_SCALE en pasos de 15 grados. */
static const int cosine_by_direction[AURABOT_DIRECTION_COUNT] = {
    1000, 966, 866, 707, 500, 259, 0, -259, -500, -707, -866, -966,
    -1000, -966, -866, -707, -500, -259, 0, 259, 500, 707, 866, 966
};

/* Valores escalados por AURABOT_TRIG_SCALE en pasos de 15 grados. */
static const int sine_by_direction[AURABOT_DIRECTION_COUNT] = {
    0, 259, 500, 707, 866, 966, 1000, 966, 866, 707, 500, 259,
    0, -259, -500, -707, -866, -966, -1000, -966, -866, -707, -500, -259
};

int angle_normalize_mrad(int angle_mrad)
{
    while (angle_mrad > AURABOT_HALF_TURN_MRAD)
        angle_mrad -= AURABOT_FULL_TURN_MRAD;
    while (angle_mrad <= -AURABOT_HALF_TURN_MRAD)
        angle_mrad += AURABOT_FULL_TURN_MRAD;
    return angle_mrad;
}

int angle_direction_index(int angle_mrad)
{
    int normalized = angle_normalize_mrad(angle_mrad);
    if (normalized < 0) normalized += AURABOT_FULL_TURN_MRAD;
    return ((normalized + AURABOT_ANGLE_HALF_STEP_MRAD) /
            AURABOT_ANGLE_STEP_MRAD) % AURABOT_DIRECTION_COUNT;
}

int angle_cosine_scaled(int angle_mrad)
{
    return cosine_by_direction[angle_direction_index(angle_mrad)];
}

int angle_sine_scaled(int angle_mrad)
{
    return sine_by_direction[angle_direction_index(angle_mrad)];
}
