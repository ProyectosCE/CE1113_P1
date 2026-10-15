import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

const cgiPath = process.argv[process.argv.indexOf("--cgi") + 1];
if (!process.argv.includes("--cgi")) throw new Error("Uso: node scripts/test-web-auth.mjs --cgi CGI");
const root = mkdtempSync(join(tmpdir(), "aurabot-auth-test-"));
const database = join(root, "users.db");
const bootstrap = join(root, "bootstrap.conf");
const sessions = join(root, "sessions");
writeFileSync(bootstrap,
    "admin:admin:$2b$12$FR6gKjCwEPYuvve8RuH4yeiRL7DN5NdTOLRP1BS/M0ycCrOAc8tYe\n",
    { mode: 0o600 });

function runCgi(method, parameters) {
    const body = method === "POST" ? Buffer.from(parameters.toString()) : Buffer.alloc(0);
    return new Promise((resolve, reject) => {
    const child = spawn(cgiPath, [], { env: { ...process.env,
        REQUEST_METHOD: method, CONTENT_LENGTH: String(body.length),
        QUERY_STRING: method === "GET" ? parameters.toString() : "",
        HTTP_COOKIE: cookie, AURABOT_WEB_AUTH_DB: database,
        AURABOT_WEB_BOOTSTRAP_FILE: bootstrap, AURABOT_WEB_SESSION_DIR: sessions } });
    let output = "", error = "";
    child.stdout.on("data", chunk => { output += chunk; });
    child.stderr.on("data", chunk => { error += chunk; });
    child.on("error", reject);
    child.stdin.on("error", streamError => {
        if (streamError.code !== "EPIPE") reject(streamError);
    });
    child.stdin.end(body);
    child.on("close", code => {
        if (code !== 0) { reject(new Error(`CGI terminó con ${code}: ${error}`)); return; }
        const [headers, content = ""] = output.split("\r\n\r\n");
        let status = 200;
        let setCookie = "";
        for (const header of headers.split("\r\n")) {
            const separator = header.indexOf(":");
            if (separator < 1) continue;
            const name = header.slice(0, separator), value = header.slice(separator + 1).trim();
            if (name.toLowerCase() === "status") status = Number.parseInt(value, 10);
            else if (name.toLowerCase() === "set-cookie") setCookie = value;
        }
        resolve({ status, setCookie, data: JSON.parse(content) });
    });
    });
}

let cookie = "", csrf = "";
async function call(op, values = {}, post = false) {
    const parameters = new URLSearchParams({ op, ...values,
        ...(post && csrf && op !== "auth-login" ? { csrf } : {}) });
    const response = await runCgi(post ? "POST" : "GET", parameters);
    if (response.setCookie) cookie = response.setCookie.split(";", 1)[0];
    return response;
}

try {
    assert.equal((await call("auth-status")).data.authenticated, false);
    assert.equal((await call("auth-login", { username: "admin", password: "incorrecta" }, true)).status, 401);
    const login = await call("auth-login", { username: "admin", password: "Test-AuraBot-2026!" }, true);
    assert.equal(login.data.role, "admin");
    csrf = login.data.csrf;
    assert.match(csrf, /^[0-9a-f]{64}$/);
    assert.equal((await call("auth-user-create",
        { username: "operador", password: "Operator-Test-2026!", role: "operator" }, true)).data.ok, true);
    const users = (await call("auth-users")).data.users;
    const operator = users.find(user => user.username === "operador");
    assert.equal(operator.role, "operator");
    assert.equal((await call("auth-user-update",
        { id: operator.id, username: "visor", role: "viewer", enabled: 1 }, true)).data.ok, true);
    assert.equal((await call("auth-user-password",
        { id: operator.id, password: "Viewer-Test-2026!" }, true)).data.ok, true);
    await call("auth-logout", {}, true);
    cookie = ""; csrf = "";
    const viewer = await call("auth-login", { username: "visor", password: "Viewer-Test-2026!" }, true);
    assert.equal(viewer.data.role, "viewer");
    csrf = viewer.data.csrf;
    assert.equal((await call("auth-users")).status, 403);
    assert.equal((await call("robot-mode", { token: "01".repeat(32), mode: 2 }, true)).status, 403);
    await call("auth-logout", {}, true);
    cookie = ""; csrf = "";
    const adminAgain = await call("auth-login",
        { username: "admin", password: "Test-AuraBot-2026!" }, true);
    csrf = adminAgain.data.csrf;
    assert.equal((await call("auth-user-delete", { id: operator.id }, true)).data.ok, true);
    const admin = (await call("auth-users")).data.users.find(user => user.username === "admin");
    assert.equal((await call("auth-user-delete", { id: admin.id }, true)).status, 409);
    console.log("Autenticación correcta: SQLite, bcrypt, CSRF, administración y roles.");
} finally {
    rmSync(root, { recursive: true, force: true });
}
