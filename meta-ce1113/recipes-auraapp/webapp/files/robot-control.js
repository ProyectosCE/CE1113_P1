(() => {
    "use strict";
    const endpoint = "/cgi-bin/operaciones.cgi";
    const byId = id => document.getElementById(id);
    const modeSwitch = byId("robot-mode");
    const moveButtons = [...document.querySelectorAll("[data-drive]")];
    // Identifica esta página, no autentica a un usuario. Cada pestaña obtiene
    // un token nuevo, incluso cuando el navegador duplica una pestaña.
    const bytes = crypto.getRandomValues(new Uint8Array(32));
    const token = [...bytes].map(value => value.toString(16).padStart(2, "0")).join("");
    let status = null;
    let connected = false;
    let ownsControl = false;
    let commandPending = false;
    let queue = Promise.resolve();
    let epoch = 0;
    let activeButton = null;
    let map = null;
    let heartbeatPending = false;
    let mutationRevision = 0;

    async function request(parameters, post = false) {
        const controller = new AbortController();
        const timer = setTimeout(() => controller.abort(), 1500);
        const body = new URLSearchParams(parameters);
        try {
            const response = await fetch(post ? endpoint : `${endpoint}?${body}`, {
                method: post ? "POST" : "GET", body: post ? body : undefined,
                cache: "no-store", signal: controller.signal
            });
            if (!response.ok) throw new Error(`HTTP ${response.status}`);
            return await response.json();
        } finally { clearTimeout(timer); }
    }

    function refreshControls() {
        const manual = connected && status?.mode === 2;
        modeSwitch.disabled = !connected || commandPending || (status?.control_busy && !ownsControl);
        byId("robot-manual-mode").disabled = modeSwitch.disabled || status?.mode === 2;
        byId("robot-claim").disabled = !manual || ownsControl || status?.control_busy || commandPending;
        byId("robot-release").disabled = !ownsControl || commandPending;
        byId("robot-stop").disabled = !ownsControl;
        const hasMotor = status?.capabilities.motors.some(Boolean);
        moveButtons.forEach(button => { button.disabled = !manual || !ownsControl || !hasMotor; });
        byId("robot-owner").textContent = !connected ? "Sin conexión" :
            ownsControl ? "Esta pestaña tiene el control" :
            status?.control_busy ? "Otra pestaña tiene el control" : "Control libre";
        byId("robot-hardware").textContent = hasMotor ?
            "Mantenga pulsada una dirección para mover; al soltar se detiene." :
            "Motores deshabilitados en la configuración de hardware; puede probar sensores y LEDs.";
    }

    function releaseBeacon() {
        if (!ownsControl) return;
        const body = new URLSearchParams({ op: "robot-release", token });
        if (!navigator.sendBeacon(endpoint, body)) {
            fetch(endpoint, { method: "POST", body, keepalive: true }).catch(() => {});
        }
    }

    function forgetControl() {
        ownsControl = false;
        activeButton?.classList.remove("pressed");
        activeButton = null;
        epoch++;
        refreshControls();
    }

    function report(data) {
        byId("status").textContent = data.ok ? "Comando aplicado" : data.error;
        byId("result").textContent = JSON.stringify(data, null, 2);
    }

    // Serializar órdenes evita que una respuesta lenta de avance llegue
    // después de la orden de detener enviada al soltar el botón.
    function command(operation, values = {}, quiet = false) {
        const queuedEpoch = epoch;
        const run = async () => {
            if (queuedEpoch !== epoch) return { ok: false, cancelled: true };
            mutationRevision++;
            try {
                const data = await request({ op: operation, token, ...values }, true);
                if (!quiet || !data.ok) report(data);
                if (!data.ok && operation !== "robot-claim") {
                    releaseBeacon();
                    forgetControl();
                }
                return data;
            } catch (error) {
                releaseBeacon();
                forgetControl();
                connected = false;
                refreshControls();
                const data = { ok: false, error: "Sin comunicación; el robot detendrá los motores al vencer el control." };
                report(data);
                return data;
            } finally { mutationRevision++; }
        };
        const result = queue.then(run, run);
        queue = result.then(() => {}, () => {});
        return result;
    }

    function drawMap() {
        if (!map || !status) return;
        const canvas = byId("robot-map");
        const context = canvas.getContext("2d");
        const cellSize = canvas.width / map.width;
        const colors = ["#e2e8f0", "#93c5fd", "#ef4444"];
        map.cells.forEach((value, index) => {
            const column = index % map.width;
            const row = Math.floor(index / map.width);
            context.fillStyle = colors[value] || colors[0];
            context.fillRect(column * cellSize, (map.height - row - 1) * cellSize, cellSize, cellSize);
        });
        const x = (status.pose.x_mm / 100 + map.width / 2 + 0.5) * cellSize;
        const y = (map.height / 2 - status.pose.y_mm / 100 - 0.5) * cellSize;
        context.save();
        context.translate(x, y);
        context.rotate(-status.pose.heading_mrad / 1000);
        context.fillStyle = "#0f172a";
        context.beginPath();
        context.moveTo(9, 0); context.lineTo(-6, -6); context.lineTo(-6, 6);
        context.closePath(); context.fill(); context.restore();
        byId("robot-map-revision").textContent = `Revisión ${map.revision} · celda de 10 cm · mapa aproximado`;
    }

    function renderStatus(data) {
        const modes = ["Inicializando", "Autónomo", "Manual", "Parada segura"];
        const autoStates = ["Inactivo", "Avanzando", "Retrocediendo", "Girando"];
        byId("robot-state").textContent = `${modes[data.mode]} · ${autoStates[data.auto_state]}`;
        if (!commandPending) modeSwitch.checked = data.mode === 1;
        byId("robot-motors").textContent = `${data.motors[0]} % / ${data.motors[1]} %`;
        byId("robot-movement").textContent = data.motor_movement.map((value, index) =>
            !data.capabilities.encoders[index] ? "Deshabilitado" :
                value ? "AVANZANDO" : "QUIETO").join(" / ");
        const directionNames = { "-1": "Reversa", "0": "Detenido", "1": "Avance" };
        byId("robot-direction").textContent = data.motor_direction.map((value, index) =>
            !data.capabilities.motors[index] ? "Deshabilitado" : directionNames[value] ?? "Desconocida").join(" / ");
        byId("robot-sensors").textContent = data.sensors.map((value, index) =>
            !data.capabilities.sensors[index] ? "Deshabilitado" : value ? "Obstáculo" : "Libre").join(" / ");
        byId("robot-pose").textContent = `${data.pose.x_mm / 10} cm, ${data.pose.y_mm / 10} cm · ${(data.pose.heading_mrad / 1000).toFixed(3)} rad`;
        byId("robot-audio").textContent = data.capabilities.audio ?
            `${["Detenido", "Reproduciendo", "Pausado", "Error"][data.audio.state]} · pista ${data.audio.track} · volumen ${data.audio.volume} %` : "Deshabilitado";
        document.querySelectorAll('[data-operation^="audio-"], #audio-volume, #audio-device-apply, #audio-track')
            .forEach(element => { element.disabled = !data.capabilities.audio; });
        if (!data.capabilities.audio) byId("audio-track-hint").textContent = "Audio deshabilitado en hardware_config.h.";
        if (data.capabilities.audio && document.activeElement !== byId("audio-volume")) {
            const volume = Math.max(0, Math.min(100, (data.audio.volume - 50) * 2));
            byId("audio-volume").value = String(volume);
            byId("audio-volume-value").textContent = `${volume}%`;
        }
        byId("robot-updated").textContent = `Actualizado: ${new Date().toLocaleTimeString()}`;
        refreshControls();
        drawMap();
    }

    // Un único ciclo sin peticiones solapadas; reintentar también permite
    // recuperar la pantalla cuando AuraBot o la red vuelven a estar disponibles.
    async function poll() {
        try {
            if (!document.hidden) {
                const revisionBeforeRead = mutationRevision;
                const data = await request({ op: "robot-status" });
                if (!data.ok) throw new Error(data.error);
                if (revisionBeforeRead !== mutationRevision || commandPending) return;
                status = data;
                connected = true;
                if (ownsControl && (data.mode !== 2 || !data.control_busy)) forgetControl();
                renderStatus(data);
                if (!map || map.revision !== data.map_revision) {
                    const snapshot = await request({ op: "robot-map" });
                    if (!snapshot.ok) throw new Error(snapshot.error);
                    map = snapshot;
                    drawMap();
                }
            }
        } catch (error) {
            releaseBeacon();
            forgetControl();
            connected = false;
            byId("robot-state").textContent = `Sin conexión: ${error.message}`;
            byId("robot-updated").textContent = "Lecturas anteriores; esperando reconexión";
            refreshControls();
        } finally { setTimeout(poll, connected ? 40 : 1000); }
    }

    byId("robot-claim").addEventListener("click", async () => {
        commandPending = true; refreshControls();
        try {
            const data = await command("robot-claim");
            ownsControl = data.ok && !document.hidden;
            if (ownsControl) status.control_busy = true;
            else if (data.ok) {
                ownsControl = true; releaseBeacon(); forgetControl();
            }
        } finally { commandPending = false; refreshControls(); }
    });

    byId("robot-release").addEventListener("click", async () => {
        commandPending = true; refreshControls();
        try { await command("robot-release"); forgetControl(); }
        finally { commandPending = false; refreshControls(); }
    });

    function stopMovement() {
        if (!activeButton) return;
        activeButton.classList.remove("pressed");
        activeButton = null;
        if (ownsControl) void command("robot-stop", {}, true);
    }

    function startMovement(button) {
        if (button.disabled || activeButton || !ownsControl) return;
        const speed = Number(byId("robot-speed").value);
        const directions = { forward: [1, 1], reverse: [-1, -1], left: [-1, 1], right: [1, -1] };
        const [left, right] = directions[button.dataset.drive];
        activeButton = button;
        button.classList.add("pressed");
        void command("robot-drive", { left: left * speed, right: right * speed }, true);
    }

    moveButtons.forEach(button => {
        button.addEventListener("pointerdown", event => {
            if (event.button !== 0) return;
            event.preventDefault();
            button.setPointerCapture(event.pointerId);
            startMovement(button);
        });
        for (const event of ["pointerup", "pointercancel", "lostpointercapture"])
            button.addEventListener(event, () => { if (activeButton === button) stopMovement(); });
        button.addEventListener("keydown", event => {
            if (![" ", "Enter"].includes(event.key)) return;
            event.preventDefault();
            if (!event.repeat) startMovement(button);
        });
        button.addEventListener("keyup", event => {
            if ([" ", "Enter"].includes(event.key)) { event.preventDefault(); stopMovement(); }
        });
        button.addEventListener("blur", stopMovement);
    });
    byId("robot-stop").addEventListener("click", () => {
        stopMovement();
        if (ownsControl) void command("robot-stop");
    });
    byId("robot-speed").addEventListener("input", event => {
        byId("robot-speed-value").textContent = `${event.target.value} %`;
    });
    async function changeMode(mode) {
        commandPending = true; refreshControls();
        stopMovement();
        try {
            const data = await command("robot-mode", { mode });
            if (data.ok) { forgetControl(); status.mode = mode; }
        } finally { commandPending = false; refreshControls(); }
    }
    modeSwitch.addEventListener("change", () => { void changeMode(modeSwitch.checked ? 1 : 2); });
    byId("robot-manual-mode").addEventListener("click", () => { void changeMode(2); });
    byId("robot-emergency-stop").addEventListener("click", async () => {
        forgetControl();
        try { report(await request({ op: "robot-emergency-stop" }, true)); }
        catch (error) { report({ ok: false, error: "No se pudo enviar la parada de emergencia" }); }
    });

    setInterval(async () => {
        if (!ownsControl || document.hidden || heartbeatPending || commandPending) return;
        heartbeatPending = true;
        try { await command("robot-heartbeat", {}, true); }
        finally { heartbeatPending = false; }
    }, 700);

    window.addEventListener("blur", stopMovement);
    document.addEventListener("visibilitychange", () => {
        if (document.hidden) { releaseBeacon(); forgetControl(); }
    });
    window.addEventListener("pagehide", () => { releaseBeacon(); forgetControl(); });
    refreshControls();
    void poll();
})();
