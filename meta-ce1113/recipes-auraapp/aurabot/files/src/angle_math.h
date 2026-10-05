#ifndef AURABOT_ANGLE_MATH_H
#define AURABOT_ANGLE_MATH_H

int angle_normalize_mrad(int angle_mrad);
int angle_direction_index(int angle_mrad);
int angle_cosine_scaled(int angle_mrad);
int angle_sine_scaled(int angle_mrad);

#endif
