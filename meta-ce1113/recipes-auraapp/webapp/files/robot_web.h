#ifndef ROBOT_WEB_H
#define ROBOT_WEB_H

int web_get_integer(const char *query, const char *name, long minimum,
                    long maximum, int *result);
void web_robot_result(const char *operation, int result);
int web_robot_handle(const char *operation, const char *query, int is_post);

#endif
