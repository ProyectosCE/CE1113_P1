#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "json_response.h"


int main(void)
{
    char *query;
    char op[32] = {0};
    float a = 0.0f;
    float b = 0.0f;
    float resultado = 0.0f;

    query = getenv("QUERY_STRING");

    if (query == NULL) {
        json_error("No se recibieron parametros");
        return 0;
    }

    sscanf(query, "op=%31[^&]&a=%f&b=%f", op, &a, &b);

    if (strcmp(op, "suma") == 0) {
        resultado = a+b;
        json_result("suma", resultado);
    }
    else if (strcmp(op, "resta") == 0) {
        resultado = a-b;
        json_result("resta", resultado);
    }
    else if (strcmp(op, "mult") == 0) {
        resultado = a*b;
        json_result("mult", resultado);
    }
    else if (strcmp(op, "div") == 0) {
        resultado = a/b;
        json_result("div", resultado);
    }
    else if (strcmp(op, "sqrt") == 0) {
        resultado = sqrt(a);
        json_result("sqrt", resultado);
    }
    else if (strcmp(op, "mensaje") == 0) {
        json_text("mensaje", "Hola desde Raspberry Pi 4");
    }
    else {
        json_error("Operacion no valida");
    }

    return 0;
}
