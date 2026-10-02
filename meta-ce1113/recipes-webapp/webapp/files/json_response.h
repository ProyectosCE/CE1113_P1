#ifndef JSON_RESPONSE_H
#define JSON_RESPONSE_H

void json_header(void);

void json_result(const char *operacion, float resultado);

void json_text(const char *operacion, const char *texto);

void json_error(const char *mensaje);

#endif
