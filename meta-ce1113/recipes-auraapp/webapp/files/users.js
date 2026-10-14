(() => {
    "use strict";

    const section = document.getElementById("user-admin");
    const form = document.getElementById("user-create-form");
    const list = document.getElementById("users-list");
    const message = document.getElementById("users-message");

    function showMessage(text, error = false) {
        message.textContent = text;
        message.classList.toggle("success-message", !error && Boolean(text));
    }

    async function request(parameters, post = false) {
        const data = await window.aurabotApi.request(parameters, { post });
        if (!data.ok) throw new Error(data.error || "La operación no se pudo completar");
        return data;
    }

    function field(label, control) {
        const wrapper = document.createElement("label");
        wrapper.className = "user-field";
        const title = document.createElement("span");
        title.textContent = label;
        wrapper.append(title, control);
        return wrapper;
    }

    function userCard(user) {
        const card = document.createElement("article");
        card.className = "user-row";
        const username = document.createElement("input");
        username.value = user.username;
        username.maxLength = 32;
        const role = document.createElement("select");
        for (const [value, label] of [["viewer", "Visor"], ["operator", "Operador"], ["admin", "Admin"]])
            role.add(new Option(label, value, false, value === user.role));
        const enabled = document.createElement("input");
        enabled.type = "checkbox";
        enabled.checked = user.enabled;
        const password = document.createElement("input");
        password.type = "password";
        password.minLength = 12;
        password.maxLength = 72;
        password.placeholder = "Nueva contraseña (opcional)";

        const actions = document.createElement("div");
        actions.className = "user-actions";
        const save = document.createElement("button");
        save.type = "button";
        save.textContent = "Guardar";
        const reset = document.createElement("button");
        reset.type = "button";
        reset.className = "secondary";
        reset.textContent = "Cambiar contraseña";
        const remove = document.createElement("button");
        remove.type = "button";
        remove.className = "danger";
        remove.textContent = "Eliminar";
        actions.append(save, reset, remove);

        save.addEventListener("click", async () => {
            try {
                await request({ op: "auth-user-update", id: user.id, username: username.value.trim(),
                    role: role.value, enabled: enabled.checked ? 1 : 0 }, true);
                showMessage("Usuario actualizado.");
                await loadUsers();
            } catch (error) { showMessage(error.message, true); }
        });
        reset.addEventListener("click", async () => {
            if (!password.reportValidity() || password.value.length === 0) {
                showMessage("Escriba una contraseña de 12 a 72 caracteres.", true);
                password.focus();
                return;
            }
            try {
                await request({ op: "auth-user-password", id: user.id, password: password.value }, true);
                password.value = "";
                showMessage("Contraseña actualizada; las sesiones anteriores fueron revocadas.");
            } catch (error) { showMessage(error.message, true); }
        });
        remove.addEventListener("click", async () => {
            if (!window.confirm(`¿Eliminar permanentemente al usuario ${user.username}?`)) return;
            try {
                await request({ op: "auth-user-delete", id: user.id }, true);
                showMessage("Usuario eliminado.");
                await loadUsers();
            } catch (error) { showMessage(error.message, true); }
        });

        card.append(field("Usuario", username), field("Rol", role), field("Activo", enabled),
            field("Restablecer contraseña", password), actions);
        return card;
    }

    async function loadUsers() {
        list.textContent = "Cargando usuarios…";
        try {
            const data = await request({ op: "auth-users" });
            list.replaceChildren(...data.users.map(userCard));
        } catch (error) {
            list.textContent = "";
            showMessage(error.message, true);
        }
    }

    form.addEventListener("submit", async event => {
        event.preventDefault();
        try {
            await request({ op: "auth-user-create",
                username: document.getElementById("new-username").value.trim(),
                password: document.getElementById("new-password").value,
                role: document.getElementById("new-role").value }, true);
            form.reset();
            showMessage("Usuario creado.");
            await loadUsers();
        } catch (error) { showMessage(error.message, true); }
    });

    window.addEventListener("aurabot:authenticated", event => {
        const admin = event.detail.role === "admin";
        section.hidden = !admin;
        if (admin) void loadUsers();
    });
    window.addEventListener("aurabot:unauthenticated", () => {
        section.hidden = true;
        list.replaceChildren();
    });
})();
