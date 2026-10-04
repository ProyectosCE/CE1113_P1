# Contrato inicial de API de AuraBot v1

Estado: **propuesta inicial para revisión**  
Versión del contrato: `1.0-draft`  
Alcance: API entre `operaciones.cgi`, `aurabotapp` y
`aurabot-orchestrator`.

## 1. Propósito

Este contrato define la interfaz pública para controlar AuraBot y consultar su
estado. Los clientes no deben incluir ni enlazar directamente `libgpio`,
`libpwm`, `libleds`, `libsensors` o `libaudio`.

```text
operaciones.cgi ─┐
                 ├── libaurabot-client ── IPC ── aurabot-orchestrator
aurabotapp ──────┘                              └── libaurabot-hal
```

El orquestador es la única fuente de verdad del modo, propiedad de movimiento,
estado aplicado a hardware, reproducción de audio y salud general.

## 2. Principios obligatorios

1. Solo la web puede solicitar cambios entre modo manual y autónomo.
2. En modo manual solo la web puede ordenar movimiento.
3. En modo autónomo solo `aurabotapp` puede ordenar movimiento.
4. Ambos clientes pueden solicitar una parada de emergencia.
5. `aurabotapp` controla los LEDs mediante esta API, nunca mediante GPIO.
6. La web controla la música y `aurabotapp` solicita efectos autónomos.
7. Un efecto autónomo interrumpe la música; el orquestador conserva y restaura
   el estado musical cuando termina el efecto.
8. El estado publicado describe valores confirmados por el orquestador, no solo
   órdenes solicitadas por un cliente.
9. Todo cambio de modo detiene los motores antes de transferir su propiedad.
10. Entrar en modo autónomo requiere que `aurabotapp` esté conectado y `READY`.

## 3. Roles

```c
typedef enum {
    AURABOT_ROLE_WEB = 1,
    AURABOT_ROLE_AUTONOMOUS = 2,
    AURABOT_ROLE_MONITOR = 3
} aurabot_role_t;
```

| Rol | Proceso esperado | Autoridad |
|---|---|---|
| `WEB` | CGI | Modo, movimiento manual, música, consultas y emergencia |
| `AUTONOMOUS` | `aurabotapp` | Movimiento autónomo, LEDs, efectos, observaciones y emergencia |
| `MONITOR` | Diagnóstico futuro | Solo estado y eventos |

El rol declarado al conectar no constituye autenticación. El orquestador debe
validarlo contra las credenciales del socket Unix (`SO_PEERCRED`) y una política
local de usuarios/grupos. Un rol no autorizado produce
`AURABOT_ERROR_FORBIDDEN`.

## 4. Estados

### 4.1 Ciclo de vida del orquestador

```c
typedef enum {
    AURABOT_LIFECYCLE_STARTING = 1,
    AURABOT_LIFECYCLE_READY,
    AURABOT_LIFECYCLE_DEGRADED,
    AURABOT_LIFECYCLE_STOPPING,
    AURABOT_LIFECYCLE_FAILED
} aurabot_lifecycle_t;
```

### 4.2 Modo operativo

```c
typedef enum {
    AURABOT_MODE_SAFE_STOP = 1,
    AURABOT_MODE_MANUAL,
    AURABOT_MODE_AUTONOMOUS
} aurabot_mode_t;
```

El arranque siempre comienza en `SAFE_STOP`. El modo no se restaura
automáticamente después de reiniciar el servicio.

Transiciones permitidas:

| Desde | Hacia | Condición |
|---|---|---|
| `SAFE_STOP` | `MANUAL` | Solicitud web válida |
| `SAFE_STOP` | `AUTONOMOUS` | Solicitud web y core `READY` |
| `MANUAL` | `AUTONOMOUS` | Solicitud web, motores detenidos y core `READY` |
| `AUTONOMOUS` | `MANUAL` | Solicitud web y motores detenidos |
| Cualquiera | `SAFE_STOP` | Emergencia, fallo crítico o pérdida del core autónomo |

### 4.3 Estado de `aurabotapp`

```c
typedef enum {
    AURABOT_CORE_DISCONNECTED = 1,
    AURABOT_CORE_INITIALIZING,
    AURABOT_CORE_READY,
    AURABOT_CORE_RUNNING,
    AURABOT_CORE_FAULT
} aurabot_core_state_t;
```

`aurabotapp` informa `INITIALIZING`, `READY` o `FAULT`. `RUNNING` lo determina
el orquestador cuando el modo autónomo está activo y el core estaba listo.
`DISCONNECTED` lo determina el orquestador al cerrarse o expirar la conexión.

### 4.4 Movimiento

```c
typedef enum {
    AURABOT_MOTION_STOPPED = 1,
    AURABOT_MOTION_MOVING,
    AURABOT_MOTION_EMERGENCY_STOP,
    AURABOT_MOTION_FAULT
} aurabot_motion_state_t;

typedef struct {
    aurabot_motion_state_t state;
    int16_t left_percent;   /* -100..100 */
    int16_t right_percent;  /* -100..100 */
    aurabot_role_t owner;   /* WEB, AUTONOMOUS o 0 si detenido */
} aurabot_motion_status_t;
```

Un porcentaje positivo significa avance y uno negativo retroceso. `0,0`
significa detenido. La interpretación física izquierda/derecha pertenece al
HAL y su configuración.

### 4.5 LEDs

```c
typedef enum {
    AURABOT_LED_SYSTEM = 1,
    AURABOT_LED_MANUAL,
    AURABOT_LED_AUTONOMOUS,
    AURABOT_LED_OBSTACLE,
    AURABOT_LED_ERROR
} aurabot_led_t;

typedef enum {
    AURABOT_LED_OFF = 1,
    AURABOT_LED_ON,
    AURABOT_LED_BLINK_SLOW,
    AURABOT_LED_BLINK_FAST
} aurabot_led_pattern_t;
```

Solamente el rol `AUTONOMOUS` solicita patrones. El orquestador publica el
patrón efectivamente aplicado. Ante desconexión del core puede aplicar por sí
mismo un patrón de error seguro.

### 4.6 Música y efectos

```c
typedef enum {
    AURABOT_MUSIC_STOPPED = 1,
    AURABOT_MUSIC_PLAYING,
    AURABOT_MUSIC_PAUSED,
    AURABOT_MUSIC_INTERRUPTED,
    AURABOT_MUSIC_FAULT
} aurabot_music_state_t;

typedef enum {
    AURABOT_EFFECT_NONE = 0,
    AURABOT_EFFECT_STARTUP,
    AURABOT_EFFECT_MODE_CHANGED,
    AURABOT_EFFECT_OBSTACLE,
    AURABOT_EFFECT_ERROR
} aurabot_effect_t;

typedef enum {
    AURABOT_EFFECT_PRIORITY_NOTIFICATION = 1,
    AURABOT_EFFECT_PRIORITY_ALERT,
    AURABOT_EFFECT_PRIORITY_CRITICAL
} aurabot_effect_priority_t;
```

Política inicial:

- `ERROR` es crítico, `OBSTACLE` es alerta y los demás son notificaciones.
- Un efecto de mayor prioridad interrumpe uno menor.
- Una solicitud duplicada ya activa o pendiente se descarta de forma exitosa.
- Mientras suena un efecto, la música figura como `INTERRUPTED`.
- Al finalizar el último efecto, una música antes `PLAYING` se reanuda; una
  música antes `PAUSED` o `STOPPED` conserva ese estado.
- Una orden web `music_stop` durante un efecto cancela la restauración.
- El volumen `0..100` es lógico; la conversión ALSA es interna.

### 4.7 Salud

```c
typedef enum {
    AURABOT_HEALTH_OK = 1,
    AURABOT_HEALTH_DEGRADED,
    AURABOT_HEALTH_CRITICAL
} aurabot_health_t;
```

Un fallo de LED o audio puede dejar el sistema `DEGRADED`. Un fallo que impida
detener/controlar motores es `CRITICAL` y fuerza `SAFE_STOP`.

## 5. Instantánea pública de estado

Los structs públicos incluyen `struct_size` para permitir extensión compatible.
Los campos reservados se inicializan en cero.

```c
#define AURABOT_API_VERSION_MAJOR 1u
#define AURABOT_API_VERSION_MINOR 0u
#define AURABOT_MAX_LEDS 5u
#define AURABOT_STATUS_MESSAGE_SIZE 128u

typedef struct {
    aurabot_led_t id;
    aurabot_led_pattern_t pattern;
} aurabot_led_status_t;

typedef struct {
    uint32_t struct_size;
    uint32_t api_version_major;
    uint32_t api_version_minor;
    uint64_t sequence;
    uint64_t monotonic_timestamp_ms;

    aurabot_lifecycle_t lifecycle;
    aurabot_mode_t mode;
    aurabot_core_state_t core_state;
    aurabot_health_t health;
    aurabot_motion_status_t motion;

    uint32_t led_count;
    aurabot_led_status_t leds[AURABOT_MAX_LEDS];
    aurabot_music_state_t music_state;
    uint32_t music_track;
    uint8_t music_volume_percent;
    aurabot_effect_t active_effect;

    int32_t last_error;
    char status_message[AURABOT_STATUS_MESSAGE_SIZE];
    uint8_t reserved[64];
} aurabot_status_t;
```

`sequence` aumenta con cada cambio publicado. El timestamp es monotónico y no
debe interpretarse como fecha civil. Los sensores se agregarán como colección
versionada cuando se confirme su inventario físico; no deben representarse por
número GPIO.

## 6. Errores

```c
typedef enum {
    AURABOT_OK = 0,

    AURABOT_ERROR_INVALID_ARGUMENT = 1,
    AURABOT_ERROR_UNSUPPORTED,
    AURABOT_ERROR_VERSION_MISMATCH,
    AURABOT_ERROR_FORBIDDEN,
    AURABOT_ERROR_WRONG_MODE,
    AURABOT_ERROR_NOT_READY,
    AURABOT_ERROR_BUSY,
    AURABOT_ERROR_CONFLICT,

    AURABOT_ERROR_UNAVAILABLE = 20,
    AURABOT_ERROR_TIMEOUT,
    AURABOT_ERROR_DISCONNECTED,
    AURABOT_ERROR_PROTOCOL,

    AURABOT_ERROR_HARDWARE = 40,
    AURABOT_ERROR_GPIO,
    AURABOT_ERROR_PWM,
    AURABOT_ERROR_AUDIO,
    AURABOT_ERROR_SENSOR,
    AURABOT_ERROR_SAFE_STOP_FAILED,

    AURABOT_ERROR_INTERNAL = 100
} aurabot_result_t;
```

Semántica mínima:

| Error | Uso |
|---|---|
| `INVALID_ARGUMENT` | Valor, enum o puntero inválido |
| `UNSUPPORTED` | Capacidad no presente en esta configuración |
| `VERSION_MISMATCH` | Cliente y orquestador incompatibles |
| `FORBIDDEN` | El rol no posee esa operación |
| `WRONG_MODE` | El rol podría operar, pero no en el modo actual |
| `NOT_READY` | Dependencia válida pero aún no preparada |
| `BUSY` | Recurso ocupado temporalmente; se puede reintentar |
| `CONFLICT` | El estado cambió y la orden ya no es aplicable |
| `UNAVAILABLE` | Orquestador no disponible al conectar |
| `TIMEOUT` | No hubo respuesta dentro del límite |
| `DISCONNECTED` | La conexión existente se perdió |
| `PROTOCOL` | Mensaje inválido o respuesta no correlacionable |
| `HARDWARE` | Fallo físico genérico sin categoría más específica |
| `SAFE_STOP_FAILED` | No se pudo confirmar una parada segura |

La biblioteca devuelve `aurabot_result_t`; no expone `errno` como contrato
público. Debe ofrecer una descripción estable y permitir recuperar detalle de
la última operación:

```c
const char *aurabot_result_string(aurabot_result_t result);

typedef struct {
    uint32_t struct_size;
    aurabot_result_t code;
    int32_t native_code;       /* Diagnóstico; no usar para lógica de negocio */
    char message[128];
} aurabot_error_info_t;

aurabot_result_t aurabot_last_error(const aurabot_client_t *client,
                                    aurabot_error_info_t *error);
```

## 7. API C pública inicial

```c
typedef struct aurabot_client aurabot_client_t;

typedef struct {
    uint32_t struct_size;
    uint32_t connect_timeout_ms;
    uint32_t request_timeout_ms;
    uint32_t reserved[8];
} aurabot_client_options_t;

aurabot_result_t aurabot_connect(aurabot_role_t role,
                                 const aurabot_client_options_t *options,
                                 aurabot_client_t **client);
void aurabot_disconnect(aurabot_client_t *client);

/* Estado y modo */
aurabot_result_t aurabot_status_get(aurabot_client_t *client,
                                    aurabot_status_t *status);
aurabot_result_t aurabot_mode_set(aurabot_client_t *client,
                                  aurabot_mode_t mode);
aurabot_result_t aurabot_core_state_set(aurabot_client_t *client,
                                        aurabot_core_state_t state);

/* Movimiento */
aurabot_result_t aurabot_drive(aurabot_client_t *client,
                               int left_percent,
                               int right_percent);
aurabot_result_t aurabot_stop(aurabot_client_t *client);
aurabot_result_t aurabot_emergency_stop(aurabot_client_t *client);

/* LEDs */
aurabot_result_t aurabot_led_set(aurabot_client_t *client,
                                 aurabot_led_t led,
                                 aurabot_led_pattern_t pattern);

/* Música web */
aurabot_result_t aurabot_music_play(aurabot_client_t *client,
                                    unsigned track);
aurabot_result_t aurabot_music_pause(aurabot_client_t *client);
aurabot_result_t aurabot_music_resume(aurabot_client_t *client);
aurabot_result_t aurabot_music_stop(aurabot_client_t *client);
aurabot_result_t aurabot_music_volume_set(aurabot_client_t *client,
                                          unsigned percent);

/* Efectos autónomos */
aurabot_result_t aurabot_effect_play(aurabot_client_t *client,
                                     aurabot_effect_t effect);
```

Los nombres de pista y el listado de música pueden mantenerse inicialmente en
la capa web. `track` es un identificador lógico validado por el orquestador, no
una ruta proporcionada por el navegador.

## 8. Eventos

La suscripción usa una conexión IPC independiente para evitar que una espera de
eventos bloquee órdenes. La implementación inicial puede ofrecer una llamada
bloqueante, fácil de integrar con `poll()`:

```c
typedef enum {
    AURABOT_EVENT_SNAPSHOT = 1,
    AURABOT_EVENT_MODE_CHANGED,
    AURABOT_EVENT_CORE_CHANGED,
    AURABOT_EVENT_MOTION_CHANGED,
    AURABOT_EVENT_LED_CHANGED,
    AURABOT_EVENT_AUDIO_CHANGED,
    AURABOT_EVENT_HEALTH_CHANGED,
    AURABOT_EVENT_ERROR
} aurabot_event_type_t;

typedef struct {
    uint32_t struct_size;
    aurabot_event_type_t type;
    uint64_t sequence;
    aurabot_status_t status; /* Instantánea consistente después del cambio */
} aurabot_event_t;

aurabot_result_t aurabot_events_connect(aurabot_client_t *client);
aurabot_result_t aurabot_event_next(aurabot_client_t *client,
                                    int timeout_ms,
                                    aurabot_event_t *event);
```

Reglas:

- El primer evento después de suscribirse es siempre `SNAPSHOT`.
- Los eventos de una conexión tienen `sequence` estrictamente creciente.
- Si el cliente detecta un salto o se reconecta, debe pedir otra instantánea.
- La web traduce estos eventos a SSE; el navegador no accede al socket Unix.
- La API v1 no promete que una misma instancia de cliente sea segura para uso
  concurrente desde varios hilos. Cada hilo usa su propia conexión o sincroniza
  externamente.

## 9. Matriz de autorización

| Operación | WEB | AUTONOMOUS | MONITOR |
|---|:---:|:---:|:---:|
| `status_get` / eventos | Sí | Sí | Sí |
| `mode_set(MANUAL/AUTONOMOUS)` | Sí | No | No |
| `core_state_set` | No | Sí | No |
| `drive` en MANUAL | Sí | No | No |
| `drive` en AUTONOMOUS | No | Sí | No |
| `stop` normal | Solo si es propietario | Solo si es propietario | No |
| `emergency_stop` | Sí | Sí | No |
| `led_set` | No | Sí | No |
| Música | Sí | No | No |
| `effect_play` | No | Sí | No |

## 10. Resultados de operaciones críticas

### `mode_set(AUTONOMOUS)`

- `OK`: motores detenidos, modo y propietario cambiados, evento publicado.
- `NOT_READY`: `aurabotapp` no está conectado y `READY`.
- `HARDWARE`: no se pudo preparar un dispositivo requerido; queda detenido.
- `SAFE_STOP_FAILED`: no se confirmó la detención; salud pasa a crítica.

### `drive(left, right)`

- `INVALID_ARGUMENT`: porcentaje fuera de `-100..100`.
- `FORBIDDEN`: rol sin autoridad de movimiento.
- `WRONG_MODE`: rol correcto en un modo que no le pertenece.
- `NOT_READY`: hardware/core no preparado.
- `OK`: orden aplicada y estado actualizado.

### `effect_play(effect)`

- `FORBIDDEN`: no es el cliente autónomo.
- `INVALID_ARGUMENT`: efecto desconocido.
- `OK`: efecto iniciado, encolado o duplicado descartado según la política.
- `AUDIO`: no pudo reproducirse; el estado musical previo debe preservarse
  cuando sea posible.

## 11. Correspondencia HTTP inicial

El CGI conserva JSON como interfaz web, pero traduce los errores de la API:

| Resultado API | HTTP sugerido |
|---|---:|
| `OK` | 200 |
| `INVALID_ARGUMENT` | 400 |
| `FORBIDDEN` | 403 |
| `WRONG_MODE`, `NOT_READY`, `CONFLICT` | 409 |
| `TIMEOUT` | 504 |
| `UNAVAILABLE`, `DISCONNECTED` | 503 |
| Error de hardware o interno | 500 |

Formato mínimo:

```json
{
  "ok": false,
  "error": {
    "code": "WRONG_MODE",
    "message": "el movimiento web requiere modo manual"
  },
  "sequence": 184
}
```

La web no debe tomar decisiones a partir del texto de `message`; usa `code`.

## 12. Aspectos pendientes antes de congelar v1

1. Confirmar inventario y nombres lógicos de LEDs y sensores.
2. Confirmar si la música debe reanudarse desde la posición exacta o reiniciar
   la pista después de un efecto.
3. Definir tiempo máximo sin señales de vida de `aurabotapp` antes de pasar a
   `SAFE_STOP`.
4. Definir aceleración/frenado y frecuencia máxima de órdenes de motores.
5. Definir usuarios/grupos Unix concretos para cada rol.
6. Decidir si se expone un modo de diagnóstico GPIO/PWM separado; no debe formar
   parte de esta API de producto.
7. Definir el inventario de efectos y su archivo/configuración asociada.

Hasta resolver estos puntos, la API es un borrador y no debe prometer
compatibilidad binaria definitiva.
