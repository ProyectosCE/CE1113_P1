(() => {
    "use strict";

    const resultElement = document.getElementById("result");
    const statusElement = document.getElementById("status");
    const byId = id => document.getElementById(id);

    function showResult(data) {
        if (!data?.ok) {
            statusElement.textContent = data?.error || "La operación produjo un error";
            resultElement.textContent = JSON.stringify(data || { error: "Sin respuesta" }, null, 2);
            return;
        }
        statusElement.textContent = data.operacion ? `Operación completada: ${data.operacion}` : "Operación completada";
        resultElement.textContent = JSON.stringify(data, null, 2);
    }

    async function executeOperation(operation) {
        const parameters = { op: operation };
        if (operation === "audio-play") {
            const track = byId("audio-track").value;
            if (track === "") { statusElement.textContent = "Seleccione una canción"; return; }
            parameters.op = "audio-track";
            parameters.track = track;
        } else if (operation === "sqrt") {
            parameters.a = byId("a").value;
        } else if (!["mensaje", "audio-pause", "audio-stop"].includes(operation)) {
            parameters.a = byId("a").value;
            parameters.b = byId("b").value;
        }

        statusElement.textContent = "Ejecutando operación…";
        try { showResult(await window.aurabotApi.request(parameters, { post: true })); }
        catch (error) {
            statusElement.textContent = "Error de comunicación";
            resultElement.textContent = error.message;
        }
    }

    async function loadPlaylist() {
        const select = byId("audio-track");
        const hint = byId("audio-track-hint");
        try {
            const data = await window.aurabotApi.request({ op: "audio-playlist" });
            if (!data.ok || !Array.isArray(data.canciones)) throw new Error(data.error || "Playlist no válida");
            select.replaceChildren();
            data.canciones.forEach((song, index) => {
                const option = document.createElement("option");
                option.value = String(index);
                option.textContent = song;
                select.appendChild(option);
            });
            if (data.canciones.length === 0) {
                select.add(new Option("No hay canciones disponibles", ""));
                hint.textContent = "Cargue archivos MP3 y genere /media/audio/playlist.txt.";
            } else hint.textContent = `${data.canciones.length} canción(es) disponibles.`;
        } catch (error) {
            select.replaceChildren(new Option("No se pudo cargar la playlist", ""));
            hint.textContent = error.message;
        }
    }

    function moveTrack(offset) {
        const select = byId("audio-track");
        if (select.options.length === 0 || select.value === "") return;
        select.selectedIndex = (select.selectedIndex + offset + select.options.length) % select.options.length;
    }

    async function loadAudioDevices() {
        const select = byId("audio-device");
        const hint = byId("audio-device-hint");
        select.replaceChildren(new Option("Buscando dispositivos ALSA…", ""));
        try {
            const data = await window.aurabotApi.request({ op: "audio-devices" });
            if (!data.ok || !Array.isArray(data.dispositivos)) {
                throw new Error(data.error || "Respuesta de dispositivos no válida");
            }
            select.replaceChildren();
            let analogOption = null;
            data.dispositivos.forEach(device => {
                const option = new Option(
                    `Tarjeta ${device.card}: ${device.id}${device.analog35 ? " — jack 3.5 mm" : ""}`,
                    String(device.card)
                );
                select.add(option);
                if (device.analog35) analogOption = option;
            });
            if (analogOption) {
                analogOption.selected = true;
                hint.textContent = "Salida analógica jack 3.5 mm detectada y seleccionada.";
            } else if (data.dispositivos.length === 0) {
                select.add(new Option("No hay dispositivos ALSA", ""));
                hint.textContent = "No se detectó ninguna tarjeta de audio.";
            } else hint.textContent = "No se identificó automáticamente una salida Headphones.";
        } catch (error) {
            select.replaceChildren(new Option("Error al consultar ALSA", ""));
            hint.textContent = error.message;
        }
    }

    async function applyAudioDevice() {
        const card = byId("audio-device").value;
        if (card === "") { statusElement.textContent = "Seleccione un dispositivo de audio"; return; }
        try { showResult(await window.aurabotApi.request({ op: "audio-device", card }, { post: true })); }
        catch (error) { showResult({ ok: false, error: error.message }); }
    }

    async function applyAudioVolume() {
        const volume = Number(byId("audio-volume").value);
        if (!Number.isInteger(volume) || volume < 0 || volume > 100) {
            statusElement.textContent = "El volumen debe estar entre 0 y 100";
            return;
        }
        try { showResult(await window.aurabotApi.request({ op: "audio-volume", volume }, { post: true })); }
        catch (error) { showResult({ ok: false, error: error.message }); }
    }

    document.querySelectorAll("[data-operation]").forEach(button => {
        button.addEventListener("click", () => executeOperation(button.dataset.operation));
    });
    byId("audio-device-apply").addEventListener("click", applyAudioDevice);
    byId("audio-device-refresh").addEventListener("click", loadAudioDevices);
    byId("audio-previous").addEventListener("click", () => moveTrack(-1));
    byId("audio-next").addEventListener("click", () => moveTrack(1));
    byId("message-button").addEventListener("click", () => executeOperation("mensaje"));

    const audioVolume = byId("audio-volume");
    audioVolume.addEventListener("input", () => {
        byId("audio-volume-value").textContent = `${audioVolume.value}%`;
    });
    audioVolume.addEventListener("change", applyAudioVolume);

    window.addEventListener("aurabot:authenticated", () => {
        statusElement.textContent = "Sesión iniciada. Esperando una operación…";
        resultElement.textContent = "—";
        void loadAudioDevices();
        void loadPlaylist();
    });
})();
