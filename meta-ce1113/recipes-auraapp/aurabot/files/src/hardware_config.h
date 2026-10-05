#ifndef AURABOT_HARDWARE_CONFIG_H
#define AURABOT_HARDWARE_CONFIG_H

/* Habilitación por dispositivo: 1 conectado, 0 omitido por completo.
 * Perfil inicial de pruebas: un infrarrojo y LED; motores/encoders/audio apagados.
 * Cambiar a 1 los dispositivos conectados antes de reconstruir la imagen. */
/* Habilita las tres señales del motor izquierdo (PWM, IN1 e IN2). */
#ifndef AURABOT_LEFT_MOTOR_ENABLE
#define AURABOT_LEFT_MOTOR_ENABLE 0
#endif
/* Habilita las tres señales del motor derecho (PWM, IN1 e IN2). */
#ifndef AURABOT_RIGHT_MOTOR_ENABLE
#define AURABOT_RIGHT_MOTOR_ENABLE 0
#endif
/* Habilita la lectura del infrarrojo izquierdo, GPIO BCM 16. */
#ifndef AURABOT_LEFT_SENSOR_ENABLE
#define AURABOT_LEFT_SENSOR_ENABLE 0
#endif
/* Habilita la lectura del infrarrojo derecho, GPIO BCM 24. */
#ifndef AURABOT_RIGHT_SENSOR_ENABLE
#define AURABOT_RIGHT_SENSOR_ENABLE 1
#endif
/* Habilita el muestreo del encoder Hall izquierdo. */
#ifndef AURABOT_LEFT_ENCODER_ENABLE
#define AURABOT_LEFT_ENCODER_ENABLE 0
#endif
/* Habilita el muestreo del encoder Hall derecho. */
#ifndef AURABOT_RIGHT_ENCODER_ENABLE
#define AURABOT_RIGHT_ENCODER_ENABLE 0
#endif
/* Habilita el LED de servicio, GPIO BCM 22. */
#ifndef AURABOT_LED_POWER_ENABLE
#define AURABOT_LED_POWER_ENABLE 1
#endif
/* Habilita el LED de modo manual, GPIO BCM 23. */
#ifndef AURABOT_LED_MANUAL_ENABLE
#define AURABOT_LED_MANUAL_ENABLE 1
#endif
/* Habilita el LED de modo autónomo, GPIO BCM 27. */
#ifndef AURABOT_LED_AUTO_ENABLE
#define AURABOT_LED_AUTO_ENABLE 1
#endif
/* Habilita el LED de obstáculo, GPIO BCM 17. */
#ifndef AURABOT_LED_OBSTACLE_ENABLE
#define AURABOT_LED_OBSTACLE_ENABLE 1
#endif
/* Habilita el acceso al servidor de audio y a su playlist. */
#ifndef AURABOT_AUDIO_ENABLE
#define AURABOT_AUDIO_ENABLE 0
#endif

/* Pin BCM de la señal PWM del motor izquierdo; -1 indica no configurado. */
#ifndef AURABOT_LEFT_PWM_PIN
#define AURABOT_LEFT_PWM_PIN (8)
#endif
/* Pin BCM IN1 del puente H del motor izquierdo; -1 indica no configurado. */
#ifndef AURABOT_LEFT_IN1_PIN
#define AURABOT_LEFT_IN1_PIN (14)
#endif
/* Pin BCM IN2 del puente H del motor izquierdo; -1 indica no configurado. */
#ifndef AURABOT_LEFT_IN2_PIN
#define AURABOT_LEFT_IN2_PIN (15)
#endif
/* Pin BCM de la señal PWM del motor derecho; -1 indica no configurado. */
#ifndef AURABOT_RIGHT_PWM_PIN
#define AURABOT_RIGHT_PWM_PIN (7)
#endif
/* Pin BCM IN1 del puente H del motor derecho; -1 indica no configurado. */
#ifndef AURABOT_RIGHT_IN1_PIN
#define AURABOT_RIGHT_IN1_PIN (10)
#endif
/* Pin BCM IN2 del puente H del motor derecho; -1 indica no configurado. */
#ifndef AURABOT_RIGHT_IN2_PIN
#define AURABOT_RIGHT_IN2_PIN (9)
#endif
/* Pin BCM del sensor infrarrojo izquierdo; -1 indica no configurado. */
#ifndef AURABOT_LEFT_SENSOR_PIN
#define AURABOT_LEFT_SENSOR_PIN (16)
#endif
/* Pin BCM del sensor infrarrojo derecho; -1 indica no configurado. */
#ifndef AURABOT_RIGHT_SENSOR_PIN
#define AURABOT_RIGHT_SENSOR_PIN (24)
#endif
/* Pin BCM del encoder Hall izquierdo; -1 indica no configurado. */
#ifndef AURABOT_LEFT_ENCODER_PIN
#define AURABOT_LEFT_ENCODER_PIN (5)
#endif
/* Pin BCM del encoder Hall derecho; -1 indica no configurado. */
#ifndef AURABOT_RIGHT_ENCODER_PIN
#define AURABOT_RIGHT_ENCODER_PIN (6)
#endif
/* Pin BCM del LED que indica alimentación/servicio activo. */
#ifndef AURABOT_LED_POWER_PIN
#define AURABOT_LED_POWER_PIN (22)
#endif
/* Pin BCM del LED que indica modo manual. */
#ifndef AURABOT_LED_MANUAL_PIN
#define AURABOT_LED_MANUAL_PIN (23)
#endif
/* Pin BCM del LED que indica modo autónomo. */
#ifndef AURABOT_LED_AUTO_PIN
#define AURABOT_LED_AUTO_PIN (27)
#endif
/* Pin BCM del LED que indica detección de obstáculo. */
#ifndef AURABOT_LED_OBSTACLE_PIN
#define AURABOT_LED_OBSTACLE_PIN (17)
#endif

/* Infrarrojo conectado: 0 detecta objeto, 1 está libre; invertir la lectura. */
#define AURABOT_SENSOR_ACTIVE_LOW 1
/* Nivel lógico activo de los LED: 1 significa activo en alto. */
#define AURABOT_LED_ACTIVE_HIGH 1
/* Frecuencia de la señal PWM aplicada a ambos motores, en hercios. */
#define AURABOT_PWM_FREQUENCY_HZ 100
/* Intervalo de lectura de los encoders Hall, en microsegundos. */
#define AURABOT_ENCODER_POLL_US 1000
/* Ticks medidos por vuelta del motor izquierdo; valor calibrable. */
#define AURABOT_LEFT_TICKS_PER_REVOLUTION 35U
/* Ticks medidos por vuelta del motor derecho; valor calibrable. */
#define AURABOT_RIGHT_TICKS_PER_REVOLUTION 35U
/* Recorrido lineal de una vuelta de la rueda izquierda, en milímetros. */
#define AURABOT_LEFT_WHEEL_CIRCUMFERENCE_MM 204U
/* Recorrido lineal de una vuelta de la rueda derecha, en milímetros. */
#define AURABOT_RIGHT_WHEEL_CIRCUMFERENCE_MM 204U
/* Separación entre los centros de las ruedas, en milímetros. */
#define AURABOT_WHEEL_BASE_MM 160U
/* Vuelta completa en microrradianes para calcular mrad/s sin coma flotante. */
#define AURABOT_FULL_TURN_MICRORAD 6283185LL
/* FIFO usado para enviar órdenes al servidor persistente de audio. */
#define AURABOT_AUDIO_CONTROL_FIFO "/run/aurabot-audio/control"
/* Lista validada de pistas disponibles para el robot. */
#define AURABOT_PLAYLIST_PATH "/media/audio/playlist.txt"

#endif
