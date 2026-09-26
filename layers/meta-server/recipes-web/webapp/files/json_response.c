#include <stdio.h>

#include "json_response.h"

void json_header(void)
{
    printf("Content-Type: application/json\r\n");
    printf("\r\n");
}

void json_result(const char *operacion, float resultado)
{
    json_header();

    printf(
        "{"
        "\"ok\":true,"
        "\"operacion\":\"%s\","
        "\"tipo\":\"numero\","
        "\"resultado\":%.2f"
        "}\n",
        operacion,
        resultado
    );
}

void json_text(const char *operacion, const char *texto)
{
    json_header();

    printf(
        "{"
        "\"ok\":true,"
        "\"operacion\":\"%s\","
        "\"tipo\":\"texto\","
        "\"resultado\":\"%s\""
        "}\n",
        operacion,
        texto
    );
}

void json_error(const char *mensaje)
{
    json_header();

    printf(
        "{"
        "\"ok\":false,"
        "\"error\":\"%s\""
        "}\n",
        mensaje
    );
}
