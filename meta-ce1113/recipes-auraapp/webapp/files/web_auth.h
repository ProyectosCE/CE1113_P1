#ifndef AURABOT_WEB_AUTH_H
#define AURABOT_WEB_AUTH_H

int web_auth_handle(const char *operation, const char *query, int is_post);
int web_auth_require(const char *query, int is_post);
int web_auth_is_operator(void);
void web_auth_forbidden(void);

#endif
