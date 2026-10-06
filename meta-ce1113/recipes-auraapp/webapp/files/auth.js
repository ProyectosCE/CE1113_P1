(() => {
    "use strict";

    const endpoint = "/cgi-bin/operaciones.cgi";
    const authScreen = document.getElementById("auth-screen");
    const appShell = document.getElementById("app-shell");
    const form = document.getElementById("auth-form");
    const message = document.getElementById("auth-message");
    const submit = document.getElementById("auth-submit");
    const usernameInput = document.getElementById("auth-username");
    const passwordInput = document.getElementById("auth-password");
    let authenticated = false;

    function showApplication(username) {
        authenticated = true;
        authScreen.hidden = true;
        appShell.hidden = false;
        document.getElementById("auth-user").textContent = username || "Sesión activa";
        message.textContent = "";
        window.dispatchEvent(new CustomEvent("aurabot:authenticated", { detail: { username } }));
    }

    function showLogin(text = "") {
        const wasAuthenticated = authenticated;
        authenticated = false;
        appShell.hidden = true;
        authScreen.hidden = false;
        message.textContent = text;
        passwordInput.value = "";
        if (wasAuthenticated) window.dispatchEvent(new Event("aurabot:unauthenticated"));
        window.setTimeout(() => (usernameInput.value ? passwordInput : usernameInput).focus(), 0);
    }

    async function request(parameters, options = {}) {
        const post = options.post === true;
        const body = new URLSearchParams(parameters);
        const response = await fetch(post ? endpoint : `${endpoint}?${body}`, {
            method: post ? "POST" : "GET",
            body: post ? body : undefined,
            cache: "no-store",
            credentials: "same-origin",
            headers: { "X-Requested-With": "AuraBot" },
            signal: options.signal
        });
        let data;
        try {
            data = await response.json();
        } catch (_) {
            throw new Error(`Respuesta no válida del servidor (HTTP ${response.status})`);
        }
        if (response.status === 401 && parameters.op !== "auth-login") {
            showLogin("La sesión venció. Inicie sesión nuevamente.");
        }
        Object.defineProperty(data, "httpStatus", { value: response.status, enumerable: false });
        return data;
    }

    window.aurabotApi = Object.freeze({
        request,
        get authenticated() { return authenticated; }
    });

    form.addEventListener("submit", async event => {
        event.preventDefault();
        submit.disabled = true;
        message.textContent = "Verificando credenciales…";
        try {
            const data = await request({
                op: "auth-login",
                username: usernameInput.value.trim(),
                password: passwordInput.value
            }, { post: true });
            if (!data.ok || !data.authenticated) {
                message.textContent = data.error || "No se pudo iniciar sesión.";
                passwordInput.select();
                return;
            }
            showApplication(data.username);
        } catch (error) {
            message.textContent = `No se pudo conectar con AuraBot: ${error.message}`;
        } finally {
            submit.disabled = false;
        }
    });

    document.getElementById("auth-logout").addEventListener("click", async () => {
        const pending = [];
        window.dispatchEvent(new CustomEvent("aurabot:before-logout", {
            detail: { waitUntil: promise => pending.push(Promise.resolve(promise)) }
        }));
        await Promise.allSettled(pending);
        try { await request({ op: "auth-logout" }, { post: true }); }
        catch (_) { /* La cookie se descarta visualmente aunque el robot esté desconectado. */ }
        showLogin("Sesión cerrada correctamente.");
    });

    (async () => {
        try {
            const data = await request({ op: "auth-status" });
            if (data.authenticated) showApplication(data.username);
            else showLogin();
        } catch (error) {
            showLogin(`No se pudo conectar con AuraBot: ${error.message}`);
        }
    })();
})();
