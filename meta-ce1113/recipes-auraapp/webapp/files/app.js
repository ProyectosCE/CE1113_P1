const API = "/cgi-bin/operaciones.cgi";

const resultElement = document.getElementById("result");
const statusElement = document.getElementById("status");
const GPIO_PINS = [4, 12, 13, 18, 19, 20, 21, 26];

async function callApi(parameters) {
    statusElement.textContent = "Enviando comando...";
    try {
        const response = await fetch(`${API}?${new URLSearchParams(parameters)}`);
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const data = await response.json();
        showResult(data);
        return data;
    } catch (error) {
        statusElement.textContent = "Error de comunicación";
        resultElement.textContent = error.message;
        return null;
    }
}

async function executeOperation(operation) {

    const a = document.getElementById("a").value;
    const b = document.getElementById("b").value;

    let url;

    if (operation === "mensaje") {

        url =
            `${API}?op=mensaje`;

    } else if (operation === "audio-play") {

        const track = document.getElementById("audio-track").value;
        if (track === "") {
            statusElement.textContent = "Seleccione una canción";
            return;
        }
        url = `${API}?op=audio-track&track=${encodeURIComponent(track)}`;

    } else if (operation.startsWith("audio-")) {

        url = `${API}?op=${encodeURIComponent(operation)}`;

    } else if (operation === "sqrt") {

        url =
            `${API}?op=sqrt` +
            `&a=${encodeURIComponent(a)}`;

    } else {

        url =
            `${API}?op=${encodeURIComponent(operation)}` +
            `&a=${encodeURIComponent(a)}` +
            `&b=${encodeURIComponent(b)}`;
    }

    statusElement.textContent =
        "Ejecutando operación...";

    try {

        const response = await fetch(url);

        if (!response.ok) {
            throw new Error(
                `HTTP ${response.status}`
            );
        }

        const data = await response.json();

        showResult(data);

    } catch (error) {

        statusElement.textContent =
            "Error de comunicación";

        resultElement.textContent =
            error.message;
    }
}

async function loadPlaylist() {
    const select = document.getElementById("audio-track");
    const hint = document.getElementById("audio-track-hint");
    try {
        const response = await fetch(`${API}?op=audio-playlist`);
        const data = await response.json();
        if (!data.ok || !Array.isArray(data.canciones)) throw new Error("Playlist no válida");
        select.innerHTML = "";
        data.canciones.forEach((song, index) => {
            const option = document.createElement("option");
            option.value = String(index);
            option.textContent = song;
            select.appendChild(option);
        });
        if (data.canciones.length === 0) {
            select.innerHTML = '<option value="">No hay canciones disponibles</option>';
            hint.textContent = "Cargue archivos MP3 y genere /media/audio/playlist.txt.";
        } else {
            hint.textContent = `${data.canciones.length} canción(es) disponibles.`;
        }
    } catch (error) {
        select.innerHTML = '<option value="">No se pudo cargar la playlist</option>';
        hint.textContent = error.message;
    }
}

function moveTrack(offset) {
    const select = document.getElementById("audio-track");
    if (select.options.length === 0 || select.value === "") return;
    select.selectedIndex = (select.selectedIndex + offset + select.options.length) % select.options.length;
}

async function loadAudioDevices() {
    const select = document.getElementById("audio-device");
    const hint = document.getElementById("audio-device-hint");

    select.innerHTML = '<option value="">Buscando dispositivos ALSA...</option>';
    try {
        const response = await fetch(`${API}?op=audio-devices`);
        const data = await response.json();
        if (!data.ok || !Array.isArray(data.dispositivos)) {
            throw new Error("Respuesta de dispositivos no válida");
        }

        select.innerHTML = "";
        let analogOption = null;
        data.dispositivos.forEach(device => {
            const option = document.createElement("option");
            option.value = String(device.card);
            option.textContent = `Tarjeta ${device.card}: ${device.id}` +
                (device.analog35 ? " — jack 3.5 mm" : "");
            select.appendChild(option);
            if (device.analog35) {
                analogOption = option;
            }
        });

        if (analogOption !== null) {
            analogOption.selected = true;
            hint.textContent = "Detectada y seleccionada la salida analógica jack 3.5 mm.";
        } else if (data.dispositivos.length === 0) {
            select.innerHTML = '<option value="">No hay dispositivos ALSA</option>';
            hint.textContent = "No se detectó ninguna tarjeta de audio.";
        } else {
            hint.textContent = "No se identificó automáticamente una salida Headphones.";
        }
    } catch (error) {
        select.innerHTML = '<option value="">Error al consultar ALSA</option>';
        hint.textContent = error.message;
    }
}

async function applyAudioDevice() {
    const select = document.getElementById("audio-device");
    if (select.value === "") {
        statusElement.textContent = "Seleccione un dispositivo de audio";
        return;
    }

    const response = await fetch(
        `${API}?op=audio-device&card=${encodeURIComponent(select.value)}`
    );
    showResult(await response.json());
}

async function applyAudioVolume() {
    const slider = document.getElementById("audio-volume");
    const volume = Number(slider.value);

    if (!Number.isInteger(volume) || volume < 0 || volume > 100) {
        statusElement.textContent = "El volumen debe estar entre 0 y 100";
        return;
    }

    try {
        const response = await fetch(
            `${API}?op=audio-volume&volume=${encodeURIComponent(volume)}`
        );
        showResult(await response.json());
    } catch (error) {
        statusElement.textContent = "Error al ajustar el volumen";
        resultElement.textContent = error.message;
    }
}

function showResult(data) {

    if (!data.ok) {

        statusElement.textContent =
            "La operación produjo un error";

        resultElement.textContent =
            JSON.stringify(data, null, 4);

        return;
    }

    statusElement.textContent =
        `Operación: ${data.operacion}`;

    resultElement.textContent =
        JSON.stringify(data, null, 4);
}


document
    .querySelectorAll("[data-operation]")
    .forEach(button => {

        button.addEventListener("click", () => {

            const operation =
                button.dataset.operation;

            executeOperation(operation);
        });
    });

document
    .getElementById("audio-device-apply")
    .addEventListener("click", applyAudioDevice);

document
    .getElementById("audio-device-refresh")
    .addEventListener("click", loadAudioDevices);

const audioVolume = document.getElementById("audio-volume");
const audioVolumeValue = document.getElementById("audio-volume-value");

audioVolume.addEventListener("input", () => {
    audioVolumeValue.textContent = `${audioVolume.value}%`;
});
audioVolume.addEventListener("change", applyAudioVolume);

document.getElementById("audio-previous").addEventListener("click", () => moveTrack(-1));
document.getElementById("audio-next").addEventListener("click", () => moveTrack(1));

function fillPinSelectors() {
    ["pwm-pin", "digital-pin"].forEach(id => {
        const select = document.getElementById(id);
        GPIO_PINS.forEach(pin => {
            const option = document.createElement("option");
            option.value = String(pin);
            option.textContent = `GPIO ${pin}`;
            select.appendChild(option);
        });
        select.value = "26";
    });
}

const pwmDuty = document.getElementById("pwm-duty");
pwmDuty.addEventListener("input", () => {
    document.getElementById("pwm-duty-value").textContent = `${pwmDuty.value}%`;
});

document.getElementById("pwm-start").addEventListener("click", () => callApi({
    op: "pwm-set",
    pin: document.getElementById("pwm-pin").value,
    frequency: document.getElementById("pwm-frequency").value,
    duty: pwmDuty.value
}));
document.getElementById("pwm-stop").addEventListener("click", () => callApi({
    op: "pwm-stop",
    pin: document.getElementById("pwm-pin").value
}));
document.getElementById("digital-on").addEventListener("click", () => callApi({
    op: "digital-write",
    pin: document.getElementById("digital-pin").value,
    value: "1"
}));
document.getElementById("digital-off").addEventListener("click", () => callApi({
    op: "digital-write",
    pin: document.getElementById("digital-pin").value,
    value: "0"
}));


document
    .getElementById("message-button")
    .addEventListener("click", () => {

        executeOperation("mensaje");
    });

fillPinSelectors();
loadAudioDevices();
loadPlaylist();
