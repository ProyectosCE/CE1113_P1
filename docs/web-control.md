# Cliente web y libaurabot.so

La ruta de control es navegador → operaciones.cgi → libaurabot.so → socket
`/run/aurabot/control.sock` → AuraBot → bibliotecas de hardware. JavaScript no
carga una biblioteca ELF: el CGI C enlaza la biblioteca compartida. El daemon
sigue siendo el único dueño de los GPIO del robot.

## Uso

1. Iniciar sesión con la cuenta configurada en la imagen.
2. Seleccionar manual con el interruptor de modo.
3. Pulsar **Tomar control**. Solo una pestaña puede adquirirlo.
4. Seleccionar velocidad y mantener pulsada una dirección. Soltar detiene los
   motores; también funciona Espacio/Enter sobre el botón seleccionado.
5. Pulsar **Liberar control** al terminar. Esto detiene los motores y libera
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
del control y es independiente de la cookie que autentica al usuario.

## Autenticación

Todas las operaciones CGI, incluidas las lecturas, requieren una sesión. El
login compara la contraseña con un hash SHA-256 salteado e iterado; la
contraseña en texto claro no se instala en el rootfs. Una sesión usa un token
aleatorio de 32 bytes en una cookie `HttpOnly`, `SameSite=Strict`, dura ocho
horas y se almacena con permisos de root bajo `/run`. Por tanto, todas las
sesiones desaparecen al reiniciar el robot.

La receta incluye credenciales únicamente para desarrollo: usuario `admin` y
contraseña `AuraBot-CE1113!`. Antes de construir una imagen entregable,
sobrescribir al menos estas variables en `conf/local.conf`:

```bitbake
AURABOT_WEB_USERNAME = "operador"
AURABOT_WEB_PASSWORD = "una-frase-larga-y-unica"
AURABOT_WEB_AUTH_SALT = "otro-valor-unico-para-este-robot"
```

La contraseña debe tener al menos 12 caracteres. El archivo final se instala
como `/etc/aurabot/web-auth.conf`, modo `0600`, y contiene únicamente usuario,
salt, iteraciones y hash. El login incorpora una demora ante credenciales
incorrectas y hace comparación del hash en tiempo constante.

Esta autenticación limita quién opera el robot y protege la contraseña
almacenada, pero el BusyBox httpd actual sirve HTTP sin TLS: alguien capaz de
capturar el tráfico de la red local todavía podría ver las credenciales o la
cookie. Usar una WLAN de confianza; para redes hostiles hace falta añadir HTTPS.

## Lecturas periódicas

El cliente espera la respuesta y programa la siguiente consulta 250 ms después.
No solapa ciclos. Tras un fallo reintenta cada segundo, marca las lecturas como
antiguas y exige adquirir el control nuevamente. No envía heartbeat desde una
pestaña oculta.

Se muestran modo, estado autónomo, potencia aplicada a cada motor, detecciones,
posición, orientación, propietario ocupado/libre, audio, los cuatro LED y
hardware habilitado.
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
| auth-status | GET | Ninguno; es la única lectura pública |
| auth-login | POST | username, password |
| auth-logout | POST | Cookie de sesión |
| robot-status | GET | Ninguno |
| robot-map | GET | Ninguno |
| robot-mode | POST | mode=1 autónomo o 2 manual, token |
| robot-claim | POST | token |
| robot-heartbeat | POST | token |
| robot-release | POST | token |
| robot-drive | POST | token, left y right entre -100 y 100 |
| robot-stop | POST | token |
| robot-emergency-stop | POST | Ninguno |

`auth-login` emite la cookie de sesión. Salvo `auth-status` y `auth-login`, todas
las rutas exigen esa cookie y responden HTTP 401 cuando falta o expiró.

`robot-status` separa la telemetría de encoder en `motor_movement` y
`motor_direction`, ambos como arreglos `[izquierdo, derecho]`. `motor_movement`
es un estado interpretado: 1 cuando se recibió un pulso durante los últimos
50 ms y 0 cuando el encoder está quieto. Dirección usa -1 para reversa, 0 para
detenido y 1 para avance.

POST usa `application/x-www-form-urlencoded`, con `op` como primer campo.
`token` son 64 caracteres hexadecimales. Se rechazan identificadores inválidos,
campos numéricos vacíos y velocidades fuera de rango. Otro propietario no puede
conducir, detener mediante robot-stop ni cambiar de modo. La parada de emergencia
es global para cualquier usuario autenticado. La interfaz y el CGI ya no exponen
operaciones manuales de PWM ni escritura GPIO por número de pin.

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

## Perfil y despliegue

El perfil del producto habilita motores, encoders, ambos sensores, cuatro LED
y audio. Para una bancada parcial, las opciones `AURABOT_*_ENABLE` permiten
deshabilitar dispositivos al configurar CMake; la interfaz refleja esas
capacidades y bloquea controles que no estén disponibles.

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
Las pruebas cubren login incorrecto/correcto, cookie y cierre de sesión, acceso
no autorizado, eliminación de PWM/GPIO, dos clientes, exclusión,
renovación/expiración, detener al liberar, mapa, audio, validación de parámetros
y parada de emergencia.
