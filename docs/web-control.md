# Cliente web y libaurabot.so

La ruta de control es navegador → operaciones.cgi → libaurabot.so → socket
`/run/aurabot/control.sock` → AuraBot → bibliotecas de hardware. JavaScript no
carga una biblioteca ELF: el CGI C enlaza la biblioteca compartida. El daemon
sigue siendo el único dueño de los GPIO del robot.

## Uso

1. Seleccionar manual con el interruptor de modo.
2. Pulsar **Tomar control**. Solo una pestaña puede adquirirlo.
3. Seleccionar velocidad y mantener pulsada una dirección. Soltar detiene los
   motores; también funciona Espacio/Enter sobre el botón seleccionado.
4. Pulsar **Liberar control** al terminar. Esto detiene los motores y libera
   inmediatamente la concesión.

Cambiar de modo o cerrar/ocultar la pestaña libera el control. Perder el foco
detiene el movimiento. Una pestaña que tenga el control renueva su concesión
cada 700 ms; AuraBot detiene los motores y libera el control al pasar 3000 ms
sin renovación. Las órdenes de movimiento y detención se serializan en el
cliente. La parada de emergencia está disponible para cualquier cliente y
entra en parada segura.
El botón **Seleccionar manual** permite salir de parada segura directamente a
manual, sin tener que activar autónomo.

Cada carga de página genera un identificador aleatorio de 32 bytes. No se
guarda ni se comparte entre pestañas. Este identificador coordina la propiedad
del control; **no es autenticación**. Todavía no hay cuentas, contraseñas ni
autorización de usuarios, conforme al alcance de esta etapa.

## Lecturas periódicas

El cliente espera la respuesta y programa la siguiente consulta 250 ms después.
No solapa ciclos. Tras un fallo reintenta cada segundo, marca las lecturas como
antiguas y exige adquirir el control nuevamente. No envía heartbeat desde una
pestaña oculta.

Se muestran modo, estado autónomo, potencia aplicada a cada motor, detecciones,
posición, orientación, propietario ocupado/libre, audio y hardware habilitado.
El mapa se descarga al cambiar su revisión y dibuja celdas desconocidas,
recorridas y obstáculos, más la posición/orientación del robot. Cada celda mide
10 cm; sigue siendo un mapa aproximado, no SLAM.

El estado de audio representa las órdenes aceptadas por AuraBot; todavía no
confirma fin de pista ni errores de reproducción del proceso de audio. Playlist,
pista, pausa, parada, volumen y selección de tarjeta pasan ahora por libaurabot.
La lista de tarjetas ALSA se consulta en el CGI.

## Contrato HTTP

Ruta: `/cgi-bin/operaciones.cgi`. Las respuestas JSON tienen `ok`; los errores
del robot incluyen `code` y `error`. No se almacenan en caché.

| Operación | Método | Parámetros adicionales |
| --- | --- | --- |
| robot-status | GET | Ninguno |
| robot-map | GET | Ninguno |
| robot-mode | POST | mode=1 autónomo o 2 manual, token |
| robot-claim | POST | token |
| robot-heartbeat | POST | token |
| robot-release | POST | token |
| robot-drive | POST | token, left y right entre -100 y 100 |
| robot-stop | POST | token |
| robot-emergency-stop | POST | Ninguno |

`robot-status` separa la telemetría de encoder en `motor_movement` y
`motor_direction`, ambos como arreglos `[izquierdo, derecho]`. `motor_movement`
es un estado interpretado: 1 cuando se recibió un pulso durante los últimos
50 ms y 0 cuando el encoder está quieto. Dirección usa -1 para reversa, 0 para
detenido y 1 para avance.

POST usa `application/x-www-form-urlencoded`, con `op` como primer campo.
`token` son 64 caracteres hexadecimales. Se rechazan identificadores inválidos,
campos numéricos vacíos y velocidades fuera de rango. Otro propietario no puede
conducir, detener mediante robot-stop ni cambiar de modo. La parada de emergencia
es global. Las operaciones GPIO de diagnóstico siguen restringidas a pines libres
y modo manual.

## Biblioteca C

El contrato público está en `recipes-lib/aurabot-api/files/include/aurabot.h`.
Se mantiene la API existente y se agregan:

- `aurabot_get_capabilities`: hardware habilitado.
- `aurabot_get_map_snapshot`: celdas y revisión de una misma respuesta.
- `aurabot_set_mode_controlled`: cambio de modo respetando al propietario.
- `aurabot_audio_set_device`: selección de tarjeta ALSA desde el daemon.

Cada petición CGI tiene su propia conexión; desconecta al terminar. La API actual
mantiene una conexión global por proceso y no es reentrante para varios hilos.
Construir daemon, biblioteca y CGI juntos en la imagen.

## Perfil parcial y despliegue

El perfil actual deja motores y encoders deshabilitados: la web lo indica y
deshabilita los botones de movimiento. Se pueden probar el sensor derecho,
LED, polling, mapa, adquisición/liberación y cambio de modo. Habilitar los
dispositivos conectados en hardware_config.h para probar movimiento físico.

```sh
./build.sh
./flash_sd.sh --device /dev/mmcblk0
```

Después de arrancar la Raspberry, recargar la página para cargar los archivos
JavaScript actualizados. El verificador de imagen exige ahora el paquete aurabot-api.

## Pruebas locales

Se verificaron las cuatro pruebas C del núcleo y hardware parcial, compilación
con bibliotecas reales, enlazado del CGI a libaurabot.so e integración HTTP.
El banco HTTP usa únicamente hardware simulado; recibe las rutas a los binarios:

```sh
node scripts/test-web-integration.mjs --daemon BIN_SIMULADO --cgi CGI
```

Compilar el daemon con `AURABOT_FAKE_HARDWARE=ON`; daemon y biblioteca deben tener
el mismo `AURABOT_RUNTIME_DIRECTORY_OVERRIDE` en /tmp. Añadir `--serve` mantiene
la web de pruebas en `http://127.0.0.1:8873`; Ctrl+C cierra daemon y servidor.
Las pruebas cubren dos clientes, exclusión, renovación/expiración, detener al
liberar, mapa, audio, validación de parámetros y parada de emergencia.
