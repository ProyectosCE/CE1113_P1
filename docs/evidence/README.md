# Evidencias de construcción y ejecución

Este directorio debe contener evidencia obtenida de la imagen final. No se deben
copiar resultados del host como si fueran mediciones de la Raspberry Pi.

## 1. Compilación cruzada

Después de `./build.sh`, localizar los logs reales:

```sh
find "$POKY_DIR/build-raspberrypi4" -path '*/temp/log.do_compile' \
  \( -path '*aurabot/*' -o -path '*aurabot-api/*' -o -path '*webapp/*' \) \
  -print
```

Copiar un fragmento que muestre compilador ARM, compilación y enlazado exitosos
a `docs/evidence/log-do-compile-arm.txt`. Añadir también la ruta del manifest y
el SHA-256 del WIC generado por `check_image.sh`.

## 2. Inicio y reinicio de servicios

En el target:

```sh
systemctl status aurabot.service aurabot-audio.service webapp-httpd.service
systemctl show aurabot.service webapp-httpd.service -p Restart -p NRestarts
systemctl kill --signal=KILL aurabot.service
sleep 3
systemctl show aurabot.service -p ActiveState -p NRestarts
journalctl -b -u aurabot.service --no-pager
```

Guardar la salida como `systemd-target.txt`. El valor `NRestarts` debe aumentar
y `ActiveState` debe volver a `active`.

## 3. Métricas

Con navegación, audio y una sesión web activos:

```sh
aurabot-metrics | tee /media/audio/aurabot-metrics.txt
```

Copiar el archivo a `docs/evidence/aurabot-metrics.txt` y resumir los valores en
el README. Si rootfs supera 200 MB o el servicio web tarda más de 15 s, añadir
una justificación técnica.

## 4. Ejecución funcional

Capturar:

- `aurabotctl status` con ambos sensores y los cuatro LEDs;
- panel web autenticado con mapa y audio;
- reproducción MP3 simultánea con navegación;
- transición autónomo/manual y reacción ante obstáculo;
- vista del circuito con aislamiento, BMS y rieles de alimentación separados.

Usar nombres descriptivos y registrar fecha, commit, imagen y condiciones de la
prueba en `target-run.md`.
