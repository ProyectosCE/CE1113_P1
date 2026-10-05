// Ejecutar con un AuraBot construido con AURABOT_FAKE_HARDWARE=ON y con
// AURABOT_RUNTIME_DIRECTORY_OVERRIDE en /tmp, compartido por libaurabot.
import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { createServer } from "node:http";
import { readFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";

const daemonPath = process.argv[process.argv.indexOf("--daemon") + 1];
const cgiPath = process.argv[process.argv.indexOf("--cgi") + 1];
if (!process.argv.includes("--daemon") || !process.argv.includes("--cgi")) {
    throw new Error("Uso: node scripts/test-web-integration.mjs --daemon BIN_SIMULADO --cgi CGI [--serve]");
}
const assetRoot = fileURLToPath(new URL("../meta-ce1113/recipes-auraapp/webapp/files/", import.meta.url));
const daemon = spawn(daemonPath, [], { stdio: ["ignore", "ignore", "pipe"] });
const server = createServer(async (request, response) => {
    const url = new URL(request.url, "http://127.0.0.1");
    if (url.pathname !== "/cgi-bin/operaciones.cgi") {
        const files = { "/": "index.html", "/index.html": "index.html", "/app.js": "app.js",
            "/robot-control.js": "robot-control.js", "/style.css": "style.css" };
        const file = files[url.pathname];
        if (!file) { response.writeHead(404).end(); return; }
        const content = await readFile(assetRoot + file);
        response.setHeader("Content-Type", file.endsWith(".js") ? "text/javascript" :
            file.endsWith(".css") ? "text/css" : "text/html");
        response.end(content);
        return;
    }
    const chunks = [];
    for await (const chunk of request) chunks.push(chunk);
    const body = Buffer.concat(chunks);
    const child = spawn(cgiPath, [], { env: { ...process.env, QUERY_STRING: url.search.slice(1),
        REQUEST_METHOD: request.method, CONTENT_LENGTH: String(body.length) } });
    let output = "";
    child.stdout.on("data", chunk => { output += chunk; });
    child.stderr.on("data", chunk => process.stderr.write(chunk));
    child.stdin.on("error", error => {
        if (error.code !== "EPIPE") console.error(error);
    });
    child.on("close", code => {
        if (code !== 0) { response.writeHead(500).end(output); return; }
        const [headers, ...content] = output.split("\r\n\r\n");
        for (const header of headers.split("\r\n")) {
            const separator = header.indexOf(":");
            if (separator > 0) response.setHeader(header.slice(0, separator), header.slice(separator + 1).trim());
        }
        response.end(content.join("\r\n\r\n"));
    });
    child.stdin.end(body);
});

function cleanup() { server.close(); daemon.kill("SIGTERM"); }
process.on("exit", () => { daemon.kill("SIGTERM"); });
process.on("SIGINT", () => { cleanup(); });
process.on("SIGTERM", () => { cleanup(); });
await new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error("AuraBot no inició")), 5000);
    daemon.stderr.on("data", chunk => {
        if (chunk.toString().includes("listo en")) { clearTimeout(timer); resolve(); }
    });
    daemon.on("exit", code => { clearTimeout(timer); reject(new Error(`AuraBot terminó: ${code}`)); });
});
await new Promise(resolve => server.listen(8873, "127.0.0.1", resolve));
const tokenA = "01".repeat(32), tokenB = "02".repeat(32);
async function get(operation) {
    return (await fetch(`http://127.0.0.1:8873/cgi-bin/operaciones.cgi?op=${operation}`)).json();
}
async function post(operation, token = tokenA, extra = {}) {
    return (await fetch("http://127.0.0.1:8873/cgi-bin/operaciones.cgi", {
        method: "POST", body: new URLSearchParams({ op: operation, token, ...extra })
    })).json();
}

try {
    assert.equal((await get("robot-status")).mode, 1);
    assert.equal((await get("robot-claim")).ok, false); // GET no modifica el robot.
    assert.equal((await post("robot-mode", tokenA, { mode: 2 })).ok, true);
    assert.equal((await post("robot-claim", "invalid")).code, -1);
    assert.equal((await post("robot-claim")).ok, true);
    assert.equal((await post("robot-claim", tokenB)).code, -4);
    assert.equal((await post("robot-drive", tokenB, { left: 35, right: 35 })).code, -2);
    assert.equal((await post("robot-mode", tokenB, { mode: 1 })).code, -4);
    assert.equal((await post("robot-drive", tokenA, { left: 101, right: 0 })).code, -1);
    assert.equal((await post("robot-drive", tokenA, { left: "", right: 0 })).code, -1);
    assert.equal((await post("robot-drive", tokenA, { left: 35, right: 35 })).ok, true);
    const moving = await get("robot-status");
    assert.deepEqual(moving.motors, [35, 35]);
    assert.deepEqual(moving.motor_movement, [1, 1]);
    assert.deepEqual(moving.motor_direction, [1, 1]);
    assert.equal(moving.control_busy, true);
    assert.deepEqual(moving.capabilities.motors, [1, 1]);
    assert.equal((await post("robot-heartbeat")).ok, true);
    assert.equal((await post("robot-stop")).ok, true);
    assert.deepEqual((await get("robot-status")).motors, [0, 0]);
    assert.equal((await post("robot-release")).ok, true);
    assert.equal((await post("robot-claim", tokenB)).ok, true);
    assert.equal((await post("robot-drive", tokenB, { left: 35, right: 35 })).ok, true);
    await new Promise(resolve => setTimeout(resolve, 3250));
    const expired = await get("robot-status");
    assert.equal(expired.control_busy, false);
    assert.deepEqual(expired.motors, [0, 0]);
    assert.equal((await post("robot-heartbeat", tokenB)).code, -2);
    const map = await get("robot-map");
    assert.equal(map.cells.length, 1600);
    assert.equal(map.width, 40);
    assert.equal(map.revision, (await get("robot-status")).map_revision);
    assert.equal((await get("audio-playlist")).canciones.length, 4);
    assert.equal((await post("audio-track", tokenA, { track: 1 })).ok, true);
    assert.equal((await get("robot-status")).audio.track, 1);
    assert.equal((await post("audio-device", tokenA, { card: 0 })).ok, true);
    assert.equal((await post("audio-stop")).ok, true);
    assert.equal((await post("robot-claim")).ok, true);
    assert.equal((await post("robot-drive", tokenA, { left: 30, right: -30 })).ok, true);
    assert.equal((await post("robot-emergency-stop", tokenB)).ok, true);
    const stopped = await get("robot-status");
    assert.equal(stopped.mode, 3);
    assert.deepEqual(stopped.motors, [0, 0]);
    assert.equal((await post("robot-mode", tokenA, { mode: 2 })).ok, true);
    console.log("Integración correcta: dos clientes, exclusión, heartbeat/expiración, motores, mapa, audio y parada de emergencia.");
    if (process.argv.includes("--serve")) {
        console.log("Vista de prueba: http://127.0.0.1:8873 · hardware simulado · Ctrl+C para cerrar");
    } else cleanup();
} catch (error) { cleanup(); throw error; }
