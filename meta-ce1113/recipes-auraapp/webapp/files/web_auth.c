#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "sha256.h"
#include "web_auth.h"

#ifndef WEB_AUTH_CONFIG_PATH
#define WEB_AUTH_CONFIG_PATH "/etc/aurabot/web-auth.conf"
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
#define MAX_PASSWORD_SIZE 256
#define MAX_SALT_SIZE 32

typedef struct {
    char username[MAX_USERNAME_SIZE + 1];
    unsigned char salt[MAX_SALT_SIZE];
    size_t salt_size;
    unsigned long iterations;
    unsigned char password_hash[SHA256_DIGEST_SIZE];
} auth_config_t;

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

static int decode_hex(const char *text, unsigned char *output, size_t capacity,
                      size_t *output_size)
{
    size_t length = strlen(text);
    if (length == 0 || (length % 2) != 0 || length / 2 > capacity) return -1;
    for (size_t index = 0; index < length / 2; ++index) {
        int high = hex_value(text[index * 2]);
        int low = hex_value(text[index * 2 + 1]);
        if (high < 0 || low < 0) return -1;
        output[index] = (unsigned char)((high << 4) | low);
    }
    *output_size = length / 2;
    return 0;
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

static int load_config(auth_config_t *config)
{
    char line[512];
    char *username, *salt, *iterations, *hash, *end;
    unsigned long parsed_iterations;
    size_t hash_size;
    FILE *file = fopen(configured_path("AURABOT_WEB_AUTH_FILE", WEB_AUTH_CONFIG_PATH), "r");
    if (file == NULL) return -1;
    if (fgets(line, sizeof(line), file) == NULL) { fclose(file); return -1; }
    fclose(file);
    line[strcspn(line, "\r\n")] = '\0';

    username = strtok(line, ":");
    salt = strtok(NULL, ":");
    iterations = strtok(NULL, ":");
    hash = strtok(NULL, ":");
    if (username == NULL || salt == NULL || iterations == NULL || hash == NULL ||
        strtok(NULL, ":") != NULL || !valid_username(username)) return -1;

    errno = 0;
    parsed_iterations = strtoul(iterations, &end, 10);
    if (errno != 0 || end == iterations || *end != '\0' ||
        parsed_iterations < 1000 || parsed_iterations > 1000000) return -1;
    if (decode_hex(salt, config->salt, sizeof(config->salt), &config->salt_size) != 0 ||
        decode_hex(hash, config->password_hash, sizeof(config->password_hash), &hash_size) != 0 ||
        hash_size != SHA256_DIGEST_SIZE) return -1;
    strcpy(config->username, username);
    config->iterations = parsed_iterations;
    return 0;
}

static void derive_password_hash(const auth_config_t *config, const char *password,
                                 unsigned char result[SHA256_DIGEST_SIZE])
{
    sha256_context_t context;
    sha256_init(&context);
    sha256_update(&context, config->salt, config->salt_size);
    sha256_update(&context, password, strlen(password));
    sha256_final(&context, result);
    for (unsigned long iteration = 1; iteration < config->iterations; ++iteration) {
        sha256_init(&context);
        sha256_update(&context, result, SHA256_DIGEST_SIZE);
        sha256_update(&context, config->salt, config->salt_size);
        sha256_final(&context, result);
    }
}

static int constant_time_equal(const unsigned char *left, const unsigned char *right,
                               size_t size)
{
    unsigned int difference = 0;
    for (size_t index = 0; index < size; ++index) difference |= left[index] ^ right[index];
    return difference == 0;
}

static int url_parameter(const char *query, const char *name, char *output, size_t capacity)
{
    size_t name_size = strlen(name);
    while (*query != '\0') {
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

static void json_auth_header(const char *status, const char *cookie)
{
    if (status != NULL) printf("Status: %s\r\n", status);
    printf("Content-Type: application/json\r\n");
    printf("Cache-Control: no-store\r\n");
    printf("X-Content-Type-Options: nosniff\r\n");
    if (cookie != NULL) printf("Set-Cookie: %s\r\n", cookie);
    printf("\r\n");
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

static int validate_session(char *username, size_t username_capacity,
                            char token[SESSION_HEX_SIZE + 1])
{
    char path[PATH_MAX], stored_username[MAX_USERNAME_SIZE + 1];
    long long expiry;
    FILE *file;
    if (read_cookie_token(token) != 0 || session_path(token, path, sizeof(path)) != 0) return 0;
    file = fopen(path, "r");
    if (file == NULL) return 0;
    if (fscanf(file, "%lld %32s", &expiry, stored_username) != 2) {
        fclose(file); unlink(path); return 0;
    }
    fclose(file);
    if (expiry < (long long)time(NULL) || !valid_username(stored_username)) {
        unlink(path); return 0;
    }
    if (username != NULL && username_capacity > 0) {
        snprintf(username, username_capacity, "%s", stored_username);
    }
    return 1;
}

static int create_session(const char *username, char token[SESSION_HEX_SIZE + 1])
{
    unsigned char random[SESSION_BYTES];
    char path[PATH_MAX];
    const char *directory = configured_path("AURABOT_WEB_SESSION_DIR", WEB_AUTH_SESSION_DIRECTORY);
    int file = -1;
    if (mkdir(directory, 0700) != 0 && errno != EEXIST) return -1;
    if (chmod(directory, 0700) != 0) return -1;
    for (unsigned int attempt = 0; attempt < 4; ++attempt) {
        if (read_random(random, sizeof(random)) != 0) return -1;
        encode_hex(random, sizeof(random), token);
        if (session_path(token, path, sizeof(path)) != 0) return -1;
        file = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (file >= 0) break;
        if (errno != EEXIST) return -1;
    }
    if (file < 0) return -1;
    char content[128];
    int length = snprintf(content, sizeof(content), "%lld %s\n",
        (long long)time(NULL) + SESSION_LIFETIME_SECONDS, username);
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
    if (read_cookie_token(token) == 0 && session_path(token, path, sizeof(path)) == 0)
        unlink(path);
}

static void print_authentication_required(void)
{
    json_auth_header("401 Unauthorized", NULL);
    printf("{\"ok\":false,\"authenticated\":false,"
           "\"error\":\"Inicie sesion para continuar\"}\n");
}

int web_auth_require(void)
{
    char token[SESSION_HEX_SIZE + 1];
    if (validate_session(NULL, 0, token)) return 1;
    print_authentication_required();
    return 0;
}

int web_auth_handle(const char *operation, const char *query, int is_post)
{
    char username[MAX_USERNAME_SIZE + 1], password[MAX_PASSWORD_SIZE + 1];
    char token[SESSION_HEX_SIZE + 1];
    auth_config_t config;

    if (strcmp(operation, "auth-status") == 0) {
        int authenticated = validate_session(username, sizeof(username), token);
        json_auth_header(NULL, NULL);
        if (authenticated)
            printf("{\"ok\":true,\"authenticated\":true,\"username\":\"%s\"}\n", username);
        else printf("{\"ok\":true,\"authenticated\":false}\n");
        return 1;
    }
    if (strcmp(operation, "auth-login") == 0) {
        unsigned char derived[SHA256_DIGEST_SIZE];
        int valid = 0;
        if (!is_post) {
            json_auth_header("405 Method Not Allowed", NULL);
            printf("{\"ok\":false,\"error\":\"Use POST para iniciar sesion\"}\n");
            return 1;
        }
        if (load_config(&config) != 0) {
            json_auth_header("503 Service Unavailable", NULL);
            printf("{\"ok\":false,\"error\":\"Autenticacion no configurada\"}\n");
            return 1;
        }
        if (url_parameter(query, "username", username, sizeof(username)) == 0 &&
            url_parameter(query, "password", password, sizeof(password)) == 0) {
            derive_password_hash(&config, password, derived);
            valid = strcmp(username, config.username) == 0 &&
                constant_time_equal(derived, config.password_hash, sizeof(derived));
            memset(derived, 0, sizeof(derived));
        }
        memset(password, 0, sizeof(password));
        if (!valid) {
            usleep(250000);
            json_auth_header("401 Unauthorized", NULL);
            printf("{\"ok\":false,\"error\":\"Usuario o contrasena incorrectos\"}\n");
            return 1;
        }
        if (create_session(config.username, token) != 0) {
            json_auth_header("503 Service Unavailable", NULL);
            printf("{\"ok\":false,\"error\":\"No se pudo crear la sesion\"}\n");
            return 1;
        }
        char cookie[192];
        snprintf(cookie, sizeof(cookie), SESSION_COOKIE "=%s; Path=/; Max-Age=%d; HttpOnly; SameSite=Strict",
                 token, SESSION_LIFETIME_SECONDS);
        json_auth_header(NULL, cookie);
        printf("{\"ok\":true,\"authenticated\":true,\"username\":\"%s\"}\n",
               config.username);
        return 1;
    }
    if (strcmp(operation, "auth-logout") == 0) {
        if (!is_post) {
            json_auth_header("405 Method Not Allowed", NULL);
            printf("{\"ok\":false,\"error\":\"Use POST para cerrar sesion\"}\n");
            return 1;
        }
        remove_current_session();
        json_auth_header(NULL, SESSION_COOKIE "=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
        printf("{\"ok\":true,\"authenticated\":false}\n");
        return 1;
    }
    return 0;
}
