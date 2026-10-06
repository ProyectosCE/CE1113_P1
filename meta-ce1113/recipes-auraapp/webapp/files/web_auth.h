#ifndef AURABOT_WEB_AUTH_H
#define AURABOT_WEB_AUTH_H

/* Atiende auth-status, auth-login y auth-logout. */
int web_auth_handle(const char *operation, const char *query, int is_post);

/* Valida la cookie de sesión e imprime HTTP 401 cuando no es válida. */
int web_auth_require(void);

#endif
