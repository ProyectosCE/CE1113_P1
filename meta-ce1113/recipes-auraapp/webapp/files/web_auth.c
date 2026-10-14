#include <ctype.h>
#include <crypt.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "web_auth.h"

#ifndef WEB_AUTH_DATABASE_PATH
#define WEB_AUTH_DATABASE_PATH "/var/lib/aurabot-auth/users.db"
#endif
#ifndef WEB_AUTH_BOOTSTRAP_PATH
#define WEB_AUTH_BOOTSTRAP_PATH "/etc/aurabot/web-auth-bootstrap.conf"
#endif
#ifndef WEB_AUTH_SESSION_DIRECTORY
#define WEB_AUTH_SESSION_DIRECTORY "/run/aurabot-web-sessions"
#endif
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define SESSION_COOKIE "aurabot_session"
#define SESSION_BYTES 32
#define SESSION_HEX_SIZE (SESSION_BYTES * 2)
#define SESSION_LIFETIME_SECONDS (8 * 60 * 60)
#define MAX_USERNAME_SIZE 32
#define MAX_PASSWORD_SIZE 72
#define MAX_ROLE_SIZE 8
#define BCRYPT_COST 12

typedef struct {
    long long id;
    long long credential_version;
    char username[MAX_USERNAME_SIZE + 1];
    char role[MAX_ROLE_SIZE + 1];
    char csrf[SESSION_HEX_SIZE + 1];
} auth_identity_t;

static auth_identity_t current_identity;
static int current_identity_valid;

static const char *configured_path(const char *environment, const char *fallback)
{
    const char *value = getenv(environment);
    return value != NULL && value[0] != '\0' ? value : fallback;
}

static int hex_value(char character)
{
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

static void encode_hex(const unsigned char *input, size_t size, char *output)
{
    static const char digits[] = "0123456789abcdef";
    for (size_t index = 0; index < size; ++index) {
        output[index * 2] = digits[input[index] >> 4];
        output[index * 2 + 1] = digits[input[index] & 0x0f];
    }
    output[size * 2] = '\0';
}

static int constant_time_equal(const char *left, const char *right)
{
    size_t left_size = strlen(left), right_size = strlen(right);
    size_t size = left_size > right_size ? left_size : right_size;
    unsigned int difference = (unsigned int)(left_size ^ right_size);
    for (size_t index = 0; index < size; ++index) {
        unsigned char a = index < left_size ? (unsigned char)left[index] : 0;
        unsigned char b = index < right_size ? (unsigned char)right[index] : 0;
        difference |= a ^ b;
    }
    return difference == 0;
}

static int read_random(unsigned char *output, size_t size)
{
    size_t complete = 0;
    int file = open("/dev/urandom", O_RDONLY);
    if (file < 0) return -1;
    while (complete < size) {
        ssize_t count = read(file, output + complete, size - complete);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { close(file); return -1; }
        complete += (size_t)count;
    }
    close(file);
    return 0;
}

static int valid_username(const char *username)
{
    size_t size = strlen(username);
    if (size == 0 || size > MAX_USERNAME_SIZE) return 0;
    for (size_t index = 0; index < size; ++index) {
        unsigned char value = (unsigned char)username[index];
        if (!isalnum(value) && value != '_' && value != '-' && value != '.') return 0;
    }
    return 1;
}

static int valid_role(const char *role)
{
    return strcmp(role, "viewer") == 0 || strcmp(role, "operator") == 0 ||
        strcmp(role, "admin") == 0;
}

static int valid_password(const char *password)
{
    size_t size = strlen(password);
    return size >= 12 && size <= MAX_PASSWORD_SIZE;
}

static int url_parameter(const char *query, const char *name, char *output, size_t capacity)
{
    size_t name_size = strlen(name);
    while (query != NULL && *query != '\0') {
        const char *end = strchr(query, '&');
        size_t field_size = end == NULL ? strlen(query) : (size_t)(end - query);
        if (field_size > name_size && strncmp(query, name, name_size) == 0 &&
            query[name_size] == '=') {
            const char *source = query + name_size + 1;
            const char *limit = query + field_size;
            size_t written = 0;
            while (source < limit) {
                unsigned char value;
                if (*source == '+') { value = ' '; source++; }
                else if (*source == '%' && source + 2 < limit) {
                    int high = hex_value(source[1]), low = hex_value(source[2]);
                    if (high < 0 || low < 0) return -1;
                    value = (unsigned char)((high << 4) | low);
                    source += 3;
                } else value = (unsigned char)*source++;
                if (value == '\0' || written + 1 >= capacity) return -1;
                output[written++] = (char)value;
            }
            if (written == 0) return -1;
            output[written] = '\0';
            return 0;
        }
        if (end == NULL) break;
        query = end + 1;
    }
    return -1;
}

static int url_integer(const char *query, const char *name, long long *value)
{
    char text[32], *end;
    if (url_parameter(query, name, text, sizeof(text)) != 0) return -1;
    errno = 0;
    *value = strtoll(text, &end, 10);
    return errno == 0 && end != text && *end == '\0' && *value > 0 ? 0 : -1;
}

static void json_auth_header(const char *status, const char *cookie)
{
    if (status != NULL) printf("Status: %s\r\n", status);
    printf("Content-Type: application/json\r\n");
    printf("Cache-Control: no-store\r\n");
    printf("X-Content-Type-Options: nosniff\r\n");
    if (cookie != NULL) printf("Set-Cookie: %s\r\n", cookie);
    printf("\r\n");
}

static void json_error_status(const char *status, const char *message)
{
    json_auth_header(status, NULL);
    printf("{\"ok\":false,\"error\":\"%s\"}\n", message);
}

static int ensure_database_directory(const char *path)
{
    char directory[PATH_MAX];
    char *separator;
    if (snprintf(directory, sizeof(directory), "%s", path) >= (int)sizeof(directory)) return -1;
    separator = strrchr(directory, '/');
    if (separator == NULL || separator == directory) return 0;
    *separator = '\0';
    if (mkdir(directory, 0700) != 0 && errno != EEXIST) return -1;
    return chmod(directory, 0700);
}

static int database_seed(sqlite3 *database)
{
    sqlite3_stmt *statement = NULL;
    FILE *file;
    char line[256], *username, *role, *password_hash;
    int count = 0, result = -1;
    if (sqlite3_prepare_v2(database, "SELECT count(*) FROM users", -1, &statement, NULL) != SQLITE_OK)
        return -1;
    if (sqlite3_step(statement) == SQLITE_ROW) count = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    if (count != 0) return 0;

    file = fopen(configured_path("AURABOT_WEB_BOOTSTRAP_FILE", WEB_AUTH_BOOTSTRAP_PATH), "r");
    if (file == NULL) return -1;
    if (fgets(line, sizeof(line), file) == NULL) { fclose(file); return -1; }
    fclose(file);
    line[strcspn(line, "\r\n")] = '\0';
    username = strtok(line, ":");
    role = strtok(NULL, ":");
    password_hash = strtok(NULL, ":");
    if (username == NULL || role == NULL || password_hash == NULL || strtok(NULL, ":") != NULL ||
        !valid_username(username) || !valid_role(role) || strncmp(password_hash, "$2b$", 4) != 0)
        return -1;
    if (sqlite3_prepare_v2(database,
        "INSERT INTO users(username,password_hash,role,created_at,updated_at) "
        "VALUES(?1,?2,?3,unixepoch(),unixepoch())", -1, &statement, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(statement, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, password_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, role, -1, SQLITE_TRANSIENT);
    result = sqlite3_step(statement) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(statement);
    return result;
}

static sqlite3 *database_open(void)
{
    static const char schema[] =
        "PRAGMA foreign_keys=ON;"
        "CREATE TABLE IF NOT EXISTS users("
        "id INTEGER PRIMARY KEY,username TEXT NOT NULL COLLATE NOCASE UNIQUE,"
        "password_hash TEXT NOT NULL,"
        "role TEXT NOT NULL CHECK(role IN ('viewer','operator','admin')),"
        "enabled INTEGER NOT NULL DEFAULT 1 CHECK(enabled IN (0,1)),"
        "credential_version INTEGER NOT NULL DEFAULT 1,"
        "created_at INTEGER NOT NULL,updated_at INTEGER NOT NULL,last_login_at INTEGER);"
        "PRAGMA user_version=1;";
    const char *path = configured_path("AURABOT_WEB_AUTH_DB", WEB_AUTH_DATABASE_PATH);
    sqlite3 *database = NULL;
    if (ensure_database_directory(path) != 0) return NULL;
    if (sqlite3_open_v2(path, &database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
        SQLITE_OPEN_FULLMUTEX, NULL) != SQLITE_OK) {
        if (database != NULL) sqlite3_close(database);
        return NULL;
    }
    sqlite3_busy_timeout(database, 1500);
    if (sqlite3_exec(database, schema, NULL, NULL, NULL) != SQLITE_OK || database_seed(database) != 0) {
        sqlite3_close(database);
        return NULL;
    }
    chmod(path, 0600);
    return database;
}

static int password_hash_create(const char *password, char output[CRYPT_OUTPUT_SIZE])
{
    unsigned char random[16];
    char setting[CRYPT_GENSALT_OUTPUT_SIZE];
    struct crypt_data data;
    char *result;
    if (!valid_password(password) || read_random(random, sizeof(random)) != 0) return -1;
    if (crypt_gensalt_rn("$2b$", BCRYPT_COST, (const char *)random, sizeof(random),
        setting, sizeof(setting)) == NULL) return -1;
    memset(&data, 0, sizeof(data));
    result = crypt_r(password, setting, &data);
    if (result == NULL || snprintf(output, CRYPT_OUTPUT_SIZE, "%s", result) >= CRYPT_OUTPUT_SIZE) {
        memset(&data, 0, sizeof(data));
        return -1;
    }
    memset(&data, 0, sizeof(data));
    return 0;
}

static int password_hash_verify(const char *password, const char *stored)
{
    struct crypt_data data;
    char *result;
    memset(&data, 0, sizeof(data));
    result = crypt_r(password, stored, &data);
    int valid = result != NULL && constant_time_equal(result, stored);
    memset(&data, 0, sizeof(data));
    return valid;
}

static int session_path(const char *token, char *path, size_t capacity)
{
    const char *directory = configured_path("AURABOT_WEB_SESSION_DIR", WEB_AUTH_SESSION_DIRECTORY);
    int size = snprintf(path, capacity, "%s/%s", directory, token);
    return size > 0 && (size_t)size < capacity ? 0 : -1;
}

static int read_cookie_token(char token[SESSION_HEX_SIZE + 1])
{
    const char *cookies = getenv("HTTP_COOKIE");
    size_t name_size = strlen(SESSION_COOKIE);
    if (cookies == NULL) return -1;
    while (*cookies != '\0') {
        while (*cookies == ' ' || *cookies == ';') cookies++;
        const char *end = strchr(cookies, ';');
        size_t size = end == NULL ? strlen(cookies) : (size_t)(end - cookies);
        if (size == name_size + 1 + SESSION_HEX_SIZE &&
            strncmp(cookies, SESSION_COOKIE, name_size) == 0 && cookies[name_size] == '=') {
            memcpy(token, cookies + name_size + 1, SESSION_HEX_SIZE);
            token[SESSION_HEX_SIZE] = '\0';
            for (size_t index = 0; index < SESSION_HEX_SIZE; ++index)
                if (hex_value(token[index]) < 0) return -1;
            return 0;
        }
        if (end == NULL) break;
        cookies = end + 1;
    }
    return -1;
}

static int validate_session(auth_identity_t *identity, char token[SESSION_HEX_SIZE + 1])
{
    char path[PATH_MAX], csrf[SESSION_HEX_SIZE + 1];
    long long expiry, user_id, credential_version;
    sqlite3 *database;
    sqlite3_stmt *statement = NULL;
    FILE *file;
    int valid = 0;
    if (read_cookie_token(token) != 0 || session_path(token, path, sizeof(path)) != 0) return 0;
    file = fopen(path, "r");
    if (file == NULL) return 0;
    if (fscanf(file, "%lld %lld %lld %64s", &expiry, &user_id, &credential_version, csrf) != 4) {
        fclose(file); unlink(path); return 0;
    }
    fclose(file);
    if (expiry < (long long)time(NULL)) { unlink(path); return 0; }
    database = database_open();
    if (database == NULL) return 0;
    if (sqlite3_prepare_v2(database,
        "SELECT username,role,credential_version FROM users WHERE id=?1 AND enabled=1",
        -1, &statement, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(statement, 1, user_id);
        if (sqlite3_step(statement) == SQLITE_ROW &&
            sqlite3_column_int64(statement, 2) == credential_version) {
            identity->id = user_id;
            identity->credential_version = credential_version;
            snprintf(identity->username, sizeof(identity->username), "%s",
                (const char *)sqlite3_column_text(statement, 0));
            snprintf(identity->role, sizeof(identity->role), "%s",
                (const char *)sqlite3_column_text(statement, 1));
            snprintf(identity->csrf, sizeof(identity->csrf), "%s", csrf);
            valid = 1;
        }
    }
    sqlite3_finalize(statement);
    sqlite3_close(database);
    if (!valid) unlink(path);
    return valid;
}

static int create_session(long long user_id, long long credential_version,
                          char token[SESSION_HEX_SIZE + 1], char csrf[SESSION_HEX_SIZE + 1])
{
    unsigned char random[SESSION_BYTES];
    char path[PATH_MAX], content[256];
    const char *directory = configured_path("AURABOT_WEB_SESSION_DIR", WEB_AUTH_SESSION_DIRECTORY);
    int file = -1;
    if (mkdir(directory, 0700) != 0 && errno != EEXIST) return -1;
    if (chmod(directory, 0700) != 0) return -1;
    if (read_random(random, sizeof(random)) != 0) return -1;
    encode_hex(random, sizeof(random), csrf);
    for (unsigned int attempt = 0; attempt < 4; ++attempt) {
        if (read_random(random, sizeof(random)) != 0) return -1;
        encode_hex(random, sizeof(random), token);
        if (session_path(token, path, sizeof(path)) != 0) return -1;
        file = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (file >= 0) break;
        if (errno != EEXIST) return -1;
    }
    if (file < 0) return -1;
    int length = snprintf(content, sizeof(content), "%lld %lld %lld %s\n",
        (long long)time(NULL) + SESSION_LIFETIME_SECONDS, user_id, credential_version, csrf);
    if (length <= 0 || (size_t)length >= sizeof(content) ||
        write(file, content, (size_t)length) != length) {
        close(file); unlink(path); return -1;
    }
    close(file);
    return 0;
}

static void remove_current_session(void)
{
    char token[SESSION_HEX_SIZE + 1], path[PATH_MAX];
    if (read_cookie_token(token) == 0 && session_path(token, path, sizeof(path)) == 0) unlink(path);
}

static int csrf_valid(const char *query, const auth_identity_t *identity)
{
    char csrf[SESSION_HEX_SIZE + 1];
    return url_parameter(query, "csrf", csrf, sizeof(csrf)) == 0 && constant_time_equal(csrf, identity->csrf);
}

static int require_admin(const char *query, int is_post, auth_identity_t *identity)
{
    char token[SESSION_HEX_SIZE + 1];
    if (!validate_session(identity, token)) {
        json_error_status("401 Unauthorized", "Inicie sesion para continuar"); return 0;
    }
    if (strcmp(identity->role, "admin") != 0) { web_auth_forbidden(); return 0; }
    if (is_post && !csrf_valid(query, identity)) {
        json_error_status("403 Forbidden", "Token de solicitud invalido"); return 0;
    }
    return 1;
}

static int active_admin_count(sqlite3 *database)
{
    sqlite3_stmt *statement = NULL;
    int count = 0;
    if (sqlite3_prepare_v2(database,
        "SELECT count(*) FROM users WHERE role='admin' AND enabled=1", -1,
        &statement, NULL) == SQLITE_OK && sqlite3_step(statement) == SQLITE_ROW)
        count = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return count;
}

static void list_users(void)
{
    sqlite3 *database = database_open();
    sqlite3_stmt *statement = NULL;
    int first = 1;
    if (database == NULL || sqlite3_prepare_v2(database,
        "SELECT id,username,role,enabled,created_at FROM users ORDER BY username COLLATE NOCASE",
        -1, &statement, NULL) != SQLITE_OK) {
        if (database != NULL) sqlite3_close(database);
        json_error_status("503 Service Unavailable", "No se pudo consultar usuarios"); return;
    }
    json_auth_header(NULL, NULL);
    printf("{\"ok\":true,\"users\":[");
    while (sqlite3_step(statement) == SQLITE_ROW) {
        printf("%s{\"id\":%lld,\"username\":\"%s\",\"role\":\"%s\","
               "\"enabled\":%s,\"created_at\":%lld}", first ? "" : ",",
            sqlite3_column_int64(statement, 0), sqlite3_column_text(statement, 1),
            sqlite3_column_text(statement, 2), sqlite3_column_int(statement, 3) ? "true" : "false",
            sqlite3_column_int64(statement, 4));
        first = 0;
    }
    printf("]}\n");
    sqlite3_finalize(statement);
    sqlite3_close(database);
}

static void create_user(const char *query)
{
    char username[MAX_USERNAME_SIZE + 1], password[MAX_PASSWORD_SIZE + 1];
    char role[MAX_ROLE_SIZE + 1], password_hash[CRYPT_OUTPUT_SIZE];
    sqlite3 *database;
    sqlite3_stmt *statement = NULL;
    if (url_parameter(query, "username", username, sizeof(username)) != 0 ||
        url_parameter(query, "password", password, sizeof(password)) != 0 ||
        url_parameter(query, "role", role, sizeof(role)) != 0 || !valid_username(username) ||
        !valid_role(role) || password_hash_create(password, password_hash) != 0) {
        memset(password, 0, sizeof(password));
        json_error_status("400 Bad Request", "Usuario, rol o contrasena no validos"); return;
    }
    memset(password, 0, sizeof(password));
    database = database_open();
    if (database == NULL || sqlite3_prepare_v2(database,
        "INSERT INTO users(username,password_hash,role,created_at,updated_at) "
        "VALUES(?1,?2,?3,unixepoch(),unixepoch())", -1, &statement, NULL) != SQLITE_OK) {
        if (database != NULL) sqlite3_close(database);
        json_error_status("503 Service Unavailable", "No se pudo abrir la base de usuarios"); return;
    }
    sqlite3_bind_text(statement, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, password_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, role, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(statement) != SQLITE_DONE) {
        sqlite3_finalize(statement); sqlite3_close(database);
        json_error_status("409 Conflict", "El nombre de usuario ya existe"); return;
    }
    sqlite3_finalize(statement); sqlite3_close(database);
    json_auth_header(NULL, NULL); printf("{\"ok\":true,\"message\":\"Usuario creado\"}\n");
}

static int load_user(sqlite3 *database, long long id, char *role, size_t role_size, int *enabled)
{
    sqlite3_stmt *statement = NULL;
    int found = 0;
    if (sqlite3_prepare_v2(database, "SELECT role,enabled FROM users WHERE id=?1", -1,
        &statement, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(statement, 1, id);
        if (sqlite3_step(statement) == SQLITE_ROW) {
            snprintf(role, role_size, "%s", (const char *)sqlite3_column_text(statement, 0));
            *enabled = sqlite3_column_int(statement, 1);
            found = 1;
        }
    }
    sqlite3_finalize(statement);
    return found;
}

static void update_user(const char *query, const auth_identity_t *administrator)
{
    long long id;
    char username[MAX_USERNAME_SIZE + 1], role[MAX_ROLE_SIZE + 1], enabled_text[4];
    char previous_role[MAX_ROLE_SIZE + 1];
    int enabled, previous_enabled;
    sqlite3 *database;
    sqlite3_stmt *statement = NULL;
    if (url_integer(query, "id", &id) != 0 ||
        url_parameter(query, "username", username, sizeof(username)) != 0 ||
        url_parameter(query, "role", role, sizeof(role)) != 0 ||
        url_parameter(query, "enabled", enabled_text, sizeof(enabled_text)) != 0 ||
        !valid_username(username) || !valid_role(role) ||
        (strcmp(enabled_text, "0") != 0 && strcmp(enabled_text, "1") != 0)) {
        json_error_status("400 Bad Request", "Datos de usuario no validos"); return;
    }
    enabled = enabled_text[0] == '1';
    if (id == administrator->id && (!enabled || strcmp(role, "admin") != 0)) {
        json_error_status("409 Conflict", "No puede desactivar ni degradar su propia cuenta"); return;
    }
    database = database_open();
    if (database == NULL || !load_user(database, id, previous_role, sizeof(previous_role), &previous_enabled)) {
        if (database != NULL) sqlite3_close(database);
        json_error_status("404 Not Found", "Usuario no encontrado"); return;
    }
    if (previous_enabled && strcmp(previous_role, "admin") == 0 &&
        (!enabled || strcmp(role, "admin") != 0) && active_admin_count(database) <= 1) {
        sqlite3_close(database);
        json_error_status("409 Conflict", "Debe existir al menos un administrador activo"); return;
    }
    if (sqlite3_prepare_v2(database,
        "UPDATE users SET username=?1,role=?2,enabled=?3,credential_version=credential_version+"
        "CASE WHEN role<>?2 OR enabled<>?3 THEN 1 ELSE 0 END,"
        "updated_at=unixepoch() WHERE id=?4 AND "
        "(NOT(role='admin' AND enabled=1 AND (?2<>'admin' OR ?3=0)) OR "
        "(SELECT count(*) FROM users WHERE role='admin' AND enabled=1)>1)",
        -1, &statement, NULL) != SQLITE_OK) {
        sqlite3_close(database); json_error_status("503 Service Unavailable", "No se pudo actualizar"); return;
    }
    sqlite3_bind_text(statement, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, role, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 3, enabled);
    sqlite3_bind_int64(statement, 4, id);
    if (sqlite3_step(statement) != SQLITE_DONE) {
        sqlite3_finalize(statement); sqlite3_close(database);
        json_error_status("409 Conflict", "El nombre de usuario ya existe"); return;
    }
    if (sqlite3_changes(database) != 1) {
        sqlite3_finalize(statement); sqlite3_close(database);
        json_error_status("409 Conflict", "Debe existir al menos un administrador activo"); return;
    }
    sqlite3_finalize(statement); sqlite3_close(database);
    json_auth_header(NULL, NULL); printf("{\"ok\":true,\"message\":\"Usuario actualizado\"}\n");
}

static void reset_password(const char *query)
{
    long long id;
    char password[MAX_PASSWORD_SIZE + 1], password_hash[CRYPT_OUTPUT_SIZE];
    sqlite3 *database;
    sqlite3_stmt *statement = NULL;
    if (url_integer(query, "id", &id) != 0 ||
        url_parameter(query, "password", password, sizeof(password)) != 0 ||
        password_hash_create(password, password_hash) != 0) {
        memset(password, 0, sizeof(password));
        json_error_status("400 Bad Request", "La contrasena debe tener entre 12 y 72 bytes"); return;
    }
    memset(password, 0, sizeof(password));
    database = database_open();
    if (database == NULL || sqlite3_prepare_v2(database,
        "UPDATE users SET password_hash=?1,credential_version=credential_version+1,"
        "updated_at=unixepoch() WHERE id=?2", -1, &statement, NULL) != SQLITE_OK) {
        if (database != NULL) sqlite3_close(database);
        json_error_status("503 Service Unavailable", "No se pudo actualizar la contrasena"); return;
    }
    sqlite3_bind_text(statement, 1, password_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(statement, 2, id);
    if (sqlite3_step(statement) != SQLITE_DONE || sqlite3_changes(database) != 1) {
        sqlite3_finalize(statement); sqlite3_close(database);
        json_error_status("404 Not Found", "Usuario no encontrado"); return;
    }
    sqlite3_finalize(statement); sqlite3_close(database);
    json_auth_header(NULL, NULL); printf("{\"ok\":true,\"message\":\"Contrasena actualizada\"}\n");
}

static void delete_user(const char *query, const auth_identity_t *administrator)
{
    long long id;
    char role[MAX_ROLE_SIZE + 1];
    int enabled;
    sqlite3 *database;
    sqlite3_stmt *statement = NULL;
    if (url_integer(query, "id", &id) != 0) {
        json_error_status("400 Bad Request", "Identificador no valido"); return;
    }
    if (id == administrator->id) {
        json_error_status("409 Conflict", "No puede eliminar su propia cuenta"); return;
    }
    database = database_open();
    if (database == NULL || !load_user(database, id, role, sizeof(role), &enabled)) {
        if (database != NULL) sqlite3_close(database);
        json_error_status("404 Not Found", "Usuario no encontrado"); return;
    }
    if (enabled && strcmp(role, "admin") == 0 && active_admin_count(database) <= 1) {
        sqlite3_close(database);
        json_error_status("409 Conflict", "No puede eliminar el ultimo administrador activo"); return;
    }
    if (sqlite3_prepare_v2(database,
        "DELETE FROM users WHERE id=?1 AND "
        "(NOT(role='admin' AND enabled=1) OR "
        "(SELECT count(*) FROM users WHERE role='admin' AND enabled=1)>1)",
        -1, &statement, NULL) != SQLITE_OK) {
        sqlite3_close(database); json_error_status("503 Service Unavailable", "No se pudo eliminar"); return;
    }
    sqlite3_bind_int64(statement, 1, id);
    if (sqlite3_step(statement) != SQLITE_DONE || sqlite3_changes(database) != 1) {
        sqlite3_finalize(statement); sqlite3_close(database);
        json_error_status("409 Conflict", "No puede eliminar el ultimo administrador activo"); return;
    }
    sqlite3_finalize(statement); sqlite3_close(database);
    json_auth_header(NULL, NULL); printf("{\"ok\":true,\"message\":\"Usuario eliminado\"}\n");
}

void web_auth_forbidden(void)
{
    json_error_status("403 Forbidden", "Su rol no permite esta operacion");
}

int web_auth_require(const char *query, int is_post)
{
    char token[SESSION_HEX_SIZE + 1];
    current_identity_valid = validate_session(&current_identity, token);
    if (!current_identity_valid) {
        json_error_status("401 Unauthorized", "Inicie sesion para continuar"); return 0;
    }
    if (is_post && !csrf_valid(query, &current_identity)) {
        json_error_status("403 Forbidden", "Token de solicitud invalido"); return 0;
    }
    return 1;
}

int web_auth_is_operator(void)
{
    return current_identity_valid && (strcmp(current_identity.role, "operator") == 0 ||
        strcmp(current_identity.role, "admin") == 0);
}

int web_auth_handle(const char *operation, const char *query, int is_post)
{
    char username[MAX_USERNAME_SIZE + 1], password[MAX_PASSWORD_SIZE + 1];
    char token[SESSION_HEX_SIZE + 1], csrf[SESSION_HEX_SIZE + 1];
    auth_identity_t identity;
    sqlite3 *database;
    sqlite3_stmt *statement = NULL;

    if (strcmp(operation, "auth-status") == 0) {
        int authenticated = validate_session(&identity, token);
        json_auth_header(NULL, NULL);
        if (authenticated)
            printf("{\"ok\":true,\"authenticated\":true,\"username\":\"%s\","
                   "\"role\":\"%s\",\"csrf\":\"%s\"}\n",
                identity.username, identity.role, identity.csrf);
        else printf("{\"ok\":true,\"authenticated\":false}\n");
        return 1;
    }
    if (strcmp(operation, "auth-login") == 0) {
        static const char dummy_hash[] =
            "$2b$12$dvkC37EIAN2S0WIYqtdIg.Cj8d6QagzRNVflEJriEUND1A9ZCa1am";
        const char *stored = dummy_hash;
        long long user_id = 0, credential_version = 0;
        int enabled = 0, found = 0, valid;
        char role[MAX_ROLE_SIZE + 1] = "viewer";
        if (!is_post) { json_error_status("405 Method Not Allowed", "Use POST para iniciar sesion"); return 1; }
        if (url_parameter(query, "username", username, sizeof(username)) != 0 ||
            url_parameter(query, "password", password, sizeof(password)) != 0) {
            json_error_status("401 Unauthorized", "Usuario o contrasena incorrectos"); return 1;
        }
        database = database_open();
        if (database == NULL) { memset(password, 0, sizeof(password));
            json_error_status("503 Service Unavailable", "Autenticacion no configurada"); return 1; }
        if (sqlite3_prepare_v2(database,
            "SELECT id,password_hash,role,enabled,credential_version FROM users WHERE username=?1",
            -1, &statement, NULL) == SQLITE_OK) {
            sqlite3_bind_text(statement, 1, username, -1, SQLITE_TRANSIENT);
            if (sqlite3_step(statement) == SQLITE_ROW) {
                user_id = sqlite3_column_int64(statement, 0);
                stored = (const char *)sqlite3_column_text(statement, 1);
                snprintf(role, sizeof(role), "%s", (const char *)sqlite3_column_text(statement, 2));
                enabled = sqlite3_column_int(statement, 3);
                credential_version = sqlite3_column_int64(statement, 4);
                found = 1;
            }
            valid = password_hash_verify(password, stored) && found && enabled;
        } else valid = 0;
        memset(password, 0, sizeof(password));
        if (!valid) {
            sqlite3_finalize(statement); sqlite3_close(database);
            usleep(250000); json_error_status("401 Unauthorized", "Usuario o contrasena incorrectos"); return 1;
        }
        sqlite3_finalize(statement);
        if (sqlite3_prepare_v2(database, "UPDATE users SET last_login_at=unixepoch() WHERE id=?1",
            -1, &statement, NULL) == SQLITE_OK) {
            sqlite3_bind_int64(statement, 1, user_id); sqlite3_step(statement);
        }
        sqlite3_finalize(statement); sqlite3_close(database);
        if (create_session(user_id, credential_version, token, csrf) != 0) {
            json_error_status("503 Service Unavailable", "No se pudo crear la sesion"); return 1;
        }
        char cookie[192];
        snprintf(cookie, sizeof(cookie), SESSION_COOKIE "=%s; Path=/; Max-Age=%d; HttpOnly; SameSite=Strict",
            token, SESSION_LIFETIME_SECONDS);
        json_auth_header(NULL, cookie);
        printf("{\"ok\":true,\"authenticated\":true,\"username\":\"%s\","
               "\"role\":\"%s\",\"csrf\":\"%s\"}\n", username, role, csrf);
        return 1;
    }
    if (strcmp(operation, "auth-logout") == 0) {
        if (!is_post) { json_error_status("405 Method Not Allowed", "Use POST para cerrar sesion"); return 1; }
        remove_current_session();
        json_auth_header(NULL, SESSION_COOKIE "=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
        printf("{\"ok\":true,\"authenticated\":false}\n");
        return 1;
    }
    if (strncmp(operation, "auth-user", 9) == 0) {
        if (!require_admin(query, is_post, &identity)) return 1;
        if (strcmp(operation, "auth-users") == 0 && !is_post) list_users();
        else if (strcmp(operation, "auth-user-create") == 0 && is_post) create_user(query);
        else if (strcmp(operation, "auth-user-update") == 0 && is_post) update_user(query, &identity);
        else if (strcmp(operation, "auth-user-password") == 0 && is_post) reset_password(query);
        else if (strcmp(operation, "auth-user-delete") == 0 && is_post) delete_user(query, &identity);
        else json_error_status("405 Method Not Allowed", "Metodo no permitido");
        return 1;
    }
    return 0;
}
