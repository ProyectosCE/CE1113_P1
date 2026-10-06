# Matriz de cumplimiento del enunciado

Esta matriz distingue implementación verificable en el repositorio de evidencia
que solo puede obtenerse en la Raspberry Pi y el robot físico. No se consideran
cumplidos los puntos físicos únicamente porque el software los configure.

| Requisito obligatorio | Implementación | Estado / evidencia pendiente |
| --- | --- | --- |
| Navegación reactiva | FSM con avance, parada, retroceso y giro en `autonomous.c`; pruebas C | Implementado; validar recorrido real |
| Dos sensores de proximidad | Izquierdo y derecho habilitados en `hardware_config.h` | Implementado; verificar montaje y polaridad |
| Dos motores, movimiento diferencial y PWM | `motion.c`, `libpwm.so`, puente H abstracto | Implementado; medir giro y aislamiento físico |
| Cuatro LEDs | Sistema, manual, autónomo y obstáculo en `state_outputs.c` | Implementado; verificar físicamente |
| MP3 concurrente | `aurabot-audio-server.service`, `libaudio.so`, ALSA/mpg123 | Implementado; verificar parlante y concurrencia |
| Lista/play/pause/stop/volumen | API, CGI y panel web | Implementado y cubierto por integración |
| Sonidos de evento | `boot`, `autonomous`, `obstacle`, `manual` | Implementado; playlist debe conservar esas cuatro entradas |
| Cambio autónomo/manual y dirección remota | Concesión exclusiva, heartbeat y parada segura | Implementado y probado con hardware simulado |
| Sensores, mapa y LEDs en tiempo real | JSON de estado + mapa incremental + canvas web | Implementado; protocolo AuraBot v3 |
| Inicio de sesión | SHA-256 salteado/iterado y cookie de sesión | Implementado; HTTP requiere una WLAN confiable |
| systemd y reinicio ante fallos | Unidades propias con `Restart=on-failure` | Implementado; capturar `NRestarts` en target |
| Biblioteca dinámica propia | `libaurabot.so` es la única interfaz usada por CGI/herramientas | Implementado |
| Desarrollo cruzado CMake/Yocto | Recetas CMake y capa `meta-ce1113` | Implementado; conservar log real de BitBake |
| Imagen mínima | Basada en `core-image-minimal`, sin GUI, apps de prueba ni el metapaquete global `kernel-modules` | Implementado; medir rootfs final |
| Métricas rootfs/boot/RAM/CPU | `aurabot-metrics` y `systemd-analyze` en la imagen | Método implementado; falta captura final en target |
| Justificación de paquetes | Tabla en `README.md` | Documentado |
| Evidencia de compilación y target | Procedimiento en `docs/evidence/README.md` | Pendiente de captura real |
| Git Flow, Issues y Conventional Commits | Rama funcional presente y validación automatizada | Parcial: los commits históricos no siguen todos Conventional Commits y los Issues deben verificarse en el servidor Git |
| Seguridad eléctrica | Reglas en `README.md` y enunciado | Requiere inspección/fotografías del circuito |
| DI1-DI4 y AC1-AC4 | Entregables documentales del equipo | Pendientes; no se infieren automáticamente |

## Requerimientos opcionales

La detección de desnivel, notificación de fin de ciclo y edición web de playlist
no se marcan como faltantes porque el enunciado los define como opcionales. La
playlist sí es persistente en la partición `AURA_AUDIO`, aunque su edición se
realiza fuera del panel mediante `load_audio.sh`.
