# Diagnóstico en la Raspberry Pi con systemd

Estos cambios requieren reconstruir la imagen e instalarla en la SD. El robot
arranca en autónomo y el perfil del producto habilita motores, encoders, ambos
sensores, los cuatro LED y audio. Puede mover los motores inmediatamente: hacer
la primera prueba con las ruedas levantadas.

## Perfil de hardware

`hardware_config.h` contiene una constante `AURABOT_*_ENABLE` por dispositivo.
Usar 1 para conectado y 0 para omitirlo. Los motores tienen un ENABLE para sus
tres señales; cada infrarrojo, encoder y LED tiene el suyo. Audio también tiene
un ENABLE para el acceso desde el controlador central.

El perfil entregable habilita el infrarrojo izquierdo (GPIO BCM 9), el derecho
(BCM 10), motores, encoders, audio y cuatro LED. Para una bancada parcial se
puede sobrescribir cada `AURABOT_*_ENABLE` desde CMake; no se debe entregar esa
configuración como imagen final.

El infrarrojo usa `AURABOT_SENSOR_ACTIVE_LOW=1`: eléctricamente entrega 0 con
obstáculo y 1 cuando está libre. La biblioteca invierte ese nivel para publicar
1 como obstáculo y 0 como libre. El sensor deshabilitado aporta 0; no se consulta su GPIO.
Los motores deshabilitados no reciben escrituras; los encoder deshabilitados
devuelven cero ticks y no se muestrean. No se inventa movimiento ni posición.

Las maniobras autónomas requieren ambos motores y encoders y al menos un
infrarrojo habilitado. Sin ellos el modo permanece en autónomo con `auto=0`
(inactivo), mientras sensores, LED, IPC y cambio de modo siguen disponibles.
Así se evita una parada por falta de ticks o por una maniobra imposible.
En manual se permiten pruebas de motores habilitados sin encoder, sin supervisión
de atasco para el motor cuyo encoder está deshabilitado.

Para probar este perfil:

```sh
aurabotctl status
```

Con los infrarrojos libres se espera `sensors=0,0`; un objeto cambia a 1 el
lado correspondiente. El LED de obstáculo GPIO 4 sigue la detección conjunta.
Cada cambio de ENABLE requiere reconstruir e instalar la imagen.

## Estado y registros

```sh
aurabotctl status
systemctl status aurabot.service
journalctl -b -u aurabot.service -f
```

`mode=1` es autónomo, `mode=2` manual y `mode=3` parada segura. `sensors=L,R`
contiene detecciones ya interpretadas: 1 significa obstáculo. Consultar estado
no cambia el modo ni las salidas. `motor_movement=L,R` usa 1 cuando hubo pulsos
del encoder durante los últimos 50 ms y 0 cuando está quieto. El campo
`movement_state=L,R` presenta esos valores como `AVANZANDO`, `QUIETO` o
`DESHABILITADO`. `motor_direction=L,R` informa por separado la orden aplicada (-1
reversa, 0 detenido, 1 avance). Un encoder de un solo canal no determina por sí
mismo la dirección. Los LED usan numeración BCM:

| GPIO | Indicación |
| --- | --- |
| 22 | Servicio activo |
| 17 | Manual |
| 27 | Autónomo |
| 4 | Obstáculo detectado en cualquiera de los dos sensores |

En parada segura se apagan los indicadores de modo; el rojo sigue indicando
la lectura de obstáculo. Si se ordena movimiento sin recibir ticks durante el
tiempo configurado, el robot entra en parada segura y registra el motivo.

## Ejecución en consola

Ejecutar cada comando por separado, sin una barra invertida al final:

```sh
systemctl stop aurabot.service
/usr/bin/aurabot
```

El programa permanece en primer plano hasta Ctrl+C. “Listo en control.sock”
significa que acepta conexiones; no es una consola interactiva. Se registran
los cambios de estado. Después de Ctrl+C:

```sh
systemctl start aurabot.service
```

Una segunda instancia es rechazada para evitar dos procesos controlando GPIO
o reemplazando el socket del primero.

## Web

La web consulta el estado real 250 ms después de cada respuesta. Activado selecciona
autónomo; desactivado selecciona manual y detiene los motores. Hay un botón
separado de parada de emergencia.

El control manual requiere pulsar «Tomar control» y mantener pulsada una dirección.
Consultar `docs/web-control.md` para el contrato, la concesión y las pruebas.

El panel web ya no expone escritura GPIO ni PWM por número de pin. Esas pruebas
de bajo nivel deben hacerse con las herramientas de diagnóstico locales; los
pines de motores, sensores, encoders y LED permanecen reservados por AuraBot.

## Sensor y LED rojo

Comparar `aurabotctl status` con el sensor libre y con un objeto cerca. Si
`sensors` cambia a 1 pero el rojo sigue apagado, revisar el journal para
errores de GPIO 4. La biblioteca ya evita reiniciar la dirección de salida
en cada ciclo y actualiza todos los LED aunque uno falle.

La polaridad permanece en `AURABOT_SENSOR_ACTIVE_LOW` de hardware_config.h.
Solo cambiarla después de verificar el nivel real del módulo: 1 corresponde
a un módulo que detecta en bajo; 0 a uno que detecta en alto. No deducirla de
una sola lectura.

## WiFi

```sh
systemctl status aurabot-wifi.service aurabot-wifi-dhcp.service
wpa_cli -i wlan0 status
ip -4 addr show dev wlan0
journalctl -b -u aurabot-wifi.service -u aurabot-wifi-dhcp.service
dmesg | tail -60
```

Sustituir wlan0 si el nombre de interfaz es distinto. `wpa_state=COMPLETED`
significa asociación completada. Si aparece COMPLETED sin IPv4, revisar DHCP
y el router. Si no aparece COMPLETED, revisar asociación, señal y credenciales
mediante los registros de wpa_supplicant. No publicar contraseñas.

`aurabot-wifi.service` mantiene `wpa_supplicant` y
`aurabot-wifi-dhcp.service` mantiene el cliente DHCP. systemd reinicia cada
proceso si falla y el journal conserva su diagnóstico.

```sh
systemctl restart aurabot-wifi.service aurabot-wifi-dhcp.service
```

Detener AuraBot no modifica la configuración WiFi ni sus enlaces de arranque.
El fallo después de desconectar alimentación requiere verificar estos registros;
el mensaje “no lease” por sí solo no identifica la causa. Apagar con `poweroff`
antes de retirar alimentación para evitar daños en el sistema de archivos.
