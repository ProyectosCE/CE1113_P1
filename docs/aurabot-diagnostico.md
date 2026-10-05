# Diagnóstico en la Raspberry Pi con SysVinit

Estos cambios requieren reconstruir la imagen e instalarla en la SD. El robot
arranca en autónomo. Con el perfil completo puede mover los motores inmediatamente;
hacer esa prueba con las ruedas levantadas. El perfil parcial actual mantiene los
motores deshabilitados.

## Hardware conectado parcialmente

`hardware_config.h` contiene una constante `AURABOT_*_ENABLE` por dispositivo.
Usar 1 para conectado y 0 para omitirlo. Los motores tienen un ENABLE para sus
tres señales; cada infrarrojo, encoder y LED tiene el suyo. Audio también tiene
un ENABLE para el acceso desde el controlador central.

El perfil actual habilita solamente el infrarrojo derecho (GPIO BCM 24) y los
cuatro LED. Los motores, encoders, infrarrojo izquierdo y audio están deshabilitados.
Deshabilitar individualmente cualquier LED que no esté conectado.

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

Con el infrarrojo libre se espera `sensors=0,0`; con un objeto, `sensors=0,1`.
El LED rojo GPIO 17 debe seguir esa detección. Los motores se muestran como
`motors=0,0`. Cada cambio de ENABLE requiere reconstruir e instalar la imagen.

## Estado y registros

```sh
aurabotctl status
tail -f /var/log/aurabot.log
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
| 23 | Manual |
| 27 | Autónomo |
| 17 | Obstáculo detectado en cualquiera de los dos sensores |

En parada segura se apagan los indicadores de modo; el rojo sigue indicando
la lectura de obstáculo. Si se ordena movimiento sin recibir ticks durante el
tiempo configurado, el robot entra en parada segura y registra el motivo.

## Ejecución en consola

Ejecutar cada comando por separado, sin una barra invertida al final:

```sh
/etc/init.d/aurabot stop
/usr/bin/aurabot
```

El programa permanece en primer plano hasta Ctrl+C. “Listo en control.sock”
significa que acepta conexiones; no es una consola interactiva. Se registran
los cambios de estado. Después de Ctrl+C:

```sh
/etc/init.d/aurabot start
```

Una segunda instancia es rechazada para evitar dos procesos controlando GPIO
o reemplazando el socket del primero.

## Web

La web consulta el estado real 40 ms después de cada respuesta. Activado selecciona
autónomo; desactivado selecciona manual y detiene los motores. Hay un botón
separado de parada de emergencia.

El control manual requiere pulsar «Tomar control» y mantener pulsada una dirección.
Consultar `docs/web-control.md` para el contrato, la concesión y las pruebas.

GPIO digital y PWM usan libaurabot y control.sock. Probar GPIO 26 en manual.
Los pines de motores, sensores, encoders y LED están reservados. No se pueden
forzar desde el panel de pruebas porque el controlador los utiliza.

## Sensor y LED rojo

Comparar `aurabotctl status` con el sensor libre y con un objeto cerca. Si
`sensors` cambia a 1 pero el rojo sigue apagado, revisar el registro para
errores de GPIO 17. La biblioteca ya evita reiniciar la dirección de salida
en cada ciclo y actualiza todos los LED aunque uno falle.

La polaridad permanece en `AURABOT_SENSOR_ACTIVE_LOW` de hardware_config.h.
Solo cambiarla después de verificar el nivel real del módulo: 1 corresponde
a un módulo que detecta en bajo; 0 a uno que detecta en alto. No deducirla de
una sola lectura.

## WiFi

```sh
/etc/init.d/wifi-init status
wpa_cli -i wlan0 status
ip -4 addr show dev wlan0
tail -40 /var/log/wifi-wpa.log
tail -40 /var/log/wifi-dhcp.log
dmesg | tail -60
```

Sustituir wlan0 si el nombre de interfaz es distinto. `wpa_state=COMPLETED`
significa asociación completada. Si aparece COMPLETED sin IPv4, revisar DHCP
y el router. Si no aparece COMPLETED, revisar asociación, señal y credenciales
mediante los registros de wpa_supplicant. No publicar contraseñas.

El arranque espera hasta 30 segundos por asociación y deja DHCP reintentando
en segundo plano. El cliente conserva la renovación de la concesión. El
script registra ambos procesos y evita duplicarlos en llamadas sucesivas.

```sh
/etc/init.d/wifi-init restart
```

Detener AuraBot no modifica la configuración WiFi ni sus enlaces de arranque.
El fallo después de desconectar alimentación requiere verificar estos registros;
el mensaje “no lease” por sí solo no identifica la causa. Apagar con `poweroff`
antes de retirar alimentación para evitar daños en el sistema de archivos.
