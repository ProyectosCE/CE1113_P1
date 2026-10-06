// Ejecutar con un AuraBot construido con AURABOT_FAKE_HARDWARE=ON y con
// AURABOT_RUNTIME_DIRECTORY_OVERRIDE en /tmp, compartido por libaurabot.
import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { createHash } from "node:crypto";
import { createServer } from "node:http";
import { mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { readFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { fileURLToPath } from "node:url";

const daemonPath = process.argv[process.argv.indexOf("--daemon") + 1];
const cgiPath = process.argv[process.argv.indexOf("--cgi") + 1];
if (!process.argv.includes("--daemon") || !process.argv.includes("--cgi")) {
    throw new Error("Uso: node scripts/test-web-integration.mjs --daemon BIN_SIMULADO --cgi CGI [--serve]");
}
const assetRoot = fileURLToPath(new URL("../meta-ce1113/recipes-auraapp/webapp/files/", import.meta.url));
const authRoot = mkdtempSync(join(tmpdir(), "aurabot-web-test-"));
const authPath = join(authRoot, "web-auth.conf");
const sessionPath = join(authRoot, "sessions");
const authUsername = "admin", authPassword = "Test-AuraBot-2026!", authIterations = 1000;
const authSalt = createHash("sha256").update("integration-test-salt").digest().subarray(0, 16);
let authDigest = createHash("sha256").update(Buffer.concat([authSalt, Buffer.from(authPassword)])).digest();
for (let index = 1; index < authIterations; ++index) {
    authDigest = createHash("sha256").update(Buffer.concat([authDigest, authSalt])).digest();
}
writeFileSync(authPath, `${authUsername}:${authSalt.toString("hex")}:${authIterations}:${authDigest.toString("hex")}\n`, { mode: 0o600 });
const daemon = spawn(daemonPath, [], { stdio: ["ignore", "ignore", "pipe"] });
const server = createServer(async (request, response) => {
    const url = new URL(request.url, "http://127.0.0.1");
    if (url.pathname !== "/cgi-bin/operaciones.cgi") {
        const files = { "/": "index.html", "/index.html": "index.html", "/app.js": "app.js",
            "/auth.js": "auth.js", "/robot-control.js": "robot-control.js", "/style.css": "style.css" };
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
        REQUEST_METHOD: request.method, CONTENT_LENGTH: String(body.length),
        HTTP_COOKIE: request.headers.cookie || "", AURABOT_WEB_AUTH_FILE: authPath,
        AURABOT_WEB_SESSION_DIR: sessionPath } });
    let output = "";
    child.stdout.on("data", chunk => { output += chunk; });
    child.stderr.on("data", chunk => process.stderr.write(chunk));
    child.stdin.on("error", error => {
        if (error.code !== "EPIPE") console.error(error);
    });
    child.on("close", code => {
        if (code !== 0) { response.writeHead(500).end(output); return; }
        const [headers, ...content] = output.split("\r\n\r\n");
        let statusCode = 200;
        for (const header of headers.split("\r\n")) {
            const separator = header.indexOf(":");
            if (separator <= 0) continue;
            const name = header.slice(0, separator);
            const value = header.slice(separator + 1).trim();
            if (name.toLowerCase() === "status") statusCode = Number.parseInt(value, 10);
            else response.setHeader(name, value);
        }
        response.statusCode = statusCode;
        response.end(content.join("\r\n\r\n"));
    });
    child.stdin.end(body);
});

function cleanup() { server.close(); daemon.kill("SIGTERM"); rmSync(authRoot, { recursive: true, force: true }); }
process.on("exit", () => { daemon.kill("SIGTERM"); rmSync(authRoot, { recursive: true, force: true }); });
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
let sessionCookie = "";
async function get(operation) {
    const response = await fetch(`http://127.0.0.1:8873/cgi-bin/operaciones.cgi?op=${operation}`, {
        headers: sessionCookie ? { cookie: sessionCookie } : {}
    });
    return { status: response.status, data: await response.json() };
}
async function post(operation, token = tokenA, extra = {}) {
    const response = await fetch("http://127.0.0.1:8873/cgi-bin/operaciones.cgi", {
        method: "POST", body: new URLSearchParams({ op: operation, token, ...extra }),
        headers: sessionCookie ? { cookie: sessionCookie } : {}
    });
    const cookie = response.headers.get("set-cookie");
    if (cookie) sessionCookie = cookie.split(";", 1)[0];
    return { status: response.status, data: await response.json() };
}

try {
    assert.equal((await get("robot-status")).status, 401);
    assert.equal((await get("auth-status")).data.authenticated, false);
    assert.equal((await post("auth-login", tokenA, { username: authUsername, password: "incorrecta" })).status, 401);
    const login = await post("auth-login", tokenA, { username: authUsername, password: authPassword });
    assert.equal(login.data.authenticated, true);
    assert.match(sessionCookie, /^aurabot_session=[0-9a-f]{64}$/);
    assert.equal((await get("auth-status")).data.authenticated, true);

    assert.equal((await get("robot-status")).data.mode, 1);
    assert.equal((await get("robot-claim")).data.ok, false); // GET no modifica el robot.
    assert.equal((await post("robot-mode", tokenA, { mode: 2 })).data.ok, true);
    assert.equal((await post("robot-claim", "invalid")).data.code, -1);
    assert.equal((await post("robot-claim")).data.ok, true);
    assert.equal((await post("robot-claim", tokenB)).data.code, -4);
    assert.equal((await post("robot-drive", tokenB, { left: 35, right: 35 })).data.code, -2);
    assert.equal((await post("robot-mode", tokenB, { mode: 1 })).data.code, -4);
    assert.equal((await post("robot-drive", tokenA, { left: 101, right: 0 })).data.code, -1);
    assert.equal((await post("robot-drive", tokenA, { left: "", right: 0 })).data.code, -1);
    assert.equal((await post("robot-drive", tokenA, { left: 35, right: 35 })).data.ok, true);
    const moving = (await get("robot-status")).data;
    assert.deepEqual(moving.motors, [35, 35]);
    assert.deepEqual(moving.motor_movement, [1, 1]);
    assert.deepEqual(moving.motor_direction, [1, 1]);
    assert.equal(moving.control_busy, true);
    assert.deepEqual(moving.capabilities.motors, [1, 1]);
    assert.deepEqual(moving.capabilities.leds, [1, 1, 1, 1]);
    assert.deepEqual(moving.leds, [1, 1, 0, 0]);
    assert.equal((await post("robot-heartbeat")).data.ok, true);
    assert.equal((await post("robot-stop")).data.ok, true);
    assert.deepEqual((await get("robot-status")).data.motors, [0, 0]);
    assert.equal((await post("robot-release")).data.ok, true);
    assert.equal((await post("robot-claim", tokenB)).data.ok, true);
    assert.equal((await post("robot-drive", tokenB, { left: 35, right: 35 })).data.ok, true);
    await new Promise(resolve => setTimeout(resolve, 3250));
    const expired = (await get("robot-status")).data;
    assert.equal(expired.control_busy, false);
    assert.deepEqual(expired.motors, [0, 0]);
    assert.equal((await post("robot-heartbeat", tokenB)).data.code, -2);
    const map = (await get("robot-map")).data;
    assert.equal(map.cells.length, 1600);
    assert.equal(map.width, 40);
    assert.equal(map.revision, (await get("robot-status")).data.map_revision);
    assert.equal((await get("audio-playlist")).data.canciones.length, 4);
    assert.equal((await post("audio-track", tokenA, { track: 1 })).data.ok, true);
    assert.equal((await get("robot-status")).data.audio.track, 1);
    assert.equal((await post("audio-device", tokenA, { card: 0 })).data.ok, true);
    assert.equal((await post("audio-stop")).data.ok, true);
    assert.equal((await post("pwm-set", tokenA, { pin: 26, frequency: 100, duty: 50 })).data.ok, false);
    assert.equal((await post("digital-write", tokenA, { pin: 26, value: 1 })).data.ok, false);
    assert.equal((await post("robot-claim")).data.ok, true);
    assert.equal((await post("robot-drive", tokenA, { left: 30, right: -30 })).data.ok, true);
    assert.equal((await post("robot-emergency-stop", tokenB)).data.ok, true);
    const stopped = (await get("robot-status")).data;
    assert.equal(stopped.mode, 3);
    assert.deepEqual(stopped.motors, [0, 0]);
    assert.equal((await post("robot-mode", tokenA, { mode: 2 })).data.ok, true);
    assert.equal((await post("auth-logout")).data.authenticated, false);
    sessionCookie = "";
    assert.equal((await get("robot-status")).status, 401);
    console.log("Integración correcta: autenticación, sesiones, exclusión, heartbeat/expiración, motores, mapa, audio y parada de emergencia.");
    if (process.argv.includes("--serve")) {
        console.log("Vista de prueba: http://127.0.0.1:8873 · hardware simulado · Ctrl+C para cerrar");
    } else cleanup();
} catch (error) { cleanup(); throw error; }
