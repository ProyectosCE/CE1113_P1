const API = "/cgi-bin/operaciones.cgi";

const resultElement = document.getElementById("result");
const statusElement = document.getElementById("status");

async function executeOperation(operation) {

    const a = document.getElementById("a").value;
    const b = document.getElementById("b").value;

    let url;

    if (operation === "mensaje") {

        url =
            `${API}?op=mensaje`;

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
    .getElementById("message-button")
    .addEventListener("click", () => {

        executeOperation("mensaje");
    });
