#ifndef AURABOT_CONFIG_H
#define AURABOT_CONFIG_H

/* Periodo máximo entre iteraciones del controlador principal. */
#define AURABOT_CONTROL_TICK_MS 25
/* Tiempo sin heartbeat tras el cual se libera el control manual. */
#define AURABOT_OWNER_LEASE_MS 3000
/* Tiempo de movimiento sin ticks antes de declarar un bloqueo mecánico. */
#define AURABOT_MOTION_STALL_MS 1000
/* Media vuelta expresada en milirradianes. */
#define AURABOT_HALF_TURN_MRAD 3142
/* Vuelta completa expresada en milirradianes. */
#define AURABOT_FULL_TURN_MRAD 6283
/* Separación angular entre entradas de las tablas trigonométricas. */
#define AURABOT_ANGLE_STEP_MRAD 262
/* Mitad del paso angular usada para redondear al índice más cercano. */
#define AURABOT_ANGLE_HALF_STEP_MRAD 131
/* Cantidad de direcciones discretas de las tablas trigonométricas. */
#define AURABOT_DIRECTION_COUNT 24
/* Escala entera usada por seno y coseno: 1000 representa 1,0. */
#define AURABOT_TRIG_SCALE 1000
/* Desplazamiento aproximado de cada sensor respecto al frente del robot. */
#define AURABOT_SENSOR_OFFSET_MRAD 785
/* Tamaño del lado de una celda del mapa, en milímetros. */
#define AURABOT_MAP_CELL_MM 100
/* Velocidad porcentual usada durante el avance autónomo. */
#define AURABOT_AUTO_FORWARD_SPEED 45
/* Magnitud porcentual usada durante el retroceso autónomo. */
#define AURABOT_AUTO_REVERSE_SPEED 35
/* Magnitud porcentual usada durante el giro autónomo. */
#define AURABOT_AUTO_TURN_SPEED 35
/* Distancia de retroceso antes de iniciar un giro, en milímetros. */
#define AURABOT_AUTO_REVERSE_DISTANCE_MM 80
/* Giro aproximado de evasión, en milirradianes. */
#define AURABOT_AUTO_TURN_ANGLE_MRAD 262
/* Tiempo máximo permitido para completar una maniobra autónoma. */
#define AURABOT_AUTO_MANEUVER_TIMEOUT_MS 3000

#endif
