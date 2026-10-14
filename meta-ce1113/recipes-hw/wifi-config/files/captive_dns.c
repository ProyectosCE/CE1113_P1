#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define DNS_PORT 53
#define DNS_MAX_PACKET 512

static volatile sig_atomic_t running = 1;

static void stop_server(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static size_t question_end(const unsigned char *packet, size_t size)
{
    size_t offset = 12;
    while (offset < size && packet[offset] != 0) {
        unsigned int label_size = packet[offset++];
        if (label_size > 63 || offset + label_size > size) return 0;
        offset += label_size;
    }
    if (offset >= size || offset + 5 > size) return 0;
    return offset + 5;
}

int main(int argc, char **argv)
{
    unsigned char request[DNS_MAX_PACKET], response[DNS_MAX_PACKET];
    struct sockaddr_in address = {0}, client;
    struct in_addr portal;
    struct timeval receive_timeout = {.tv_sec = 1, .tv_usec = 0};
    socklen_t client_size;
    int server, enabled = 1;
    if (argc != 3 || inet_pton(AF_INET, argv[2], &portal) != 1) {
        fprintf(stderr, "uso: %s INTERFAZ IP\n", argv[0]);
        return EXIT_FAILURE;
    }
    server = socket(AF_INET, SOCK_DGRAM, 0);
    if (server < 0 || setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)) != 0 ||
        setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, &receive_timeout, sizeof(receive_timeout)) != 0 ||
        setsockopt(server, SOL_SOCKET, SO_BINDTODEVICE, argv[1], strlen(argv[1]) + 1) != 0) {
        perror("socket DNS");
        return EXIT_FAILURE;
    }
    address.sin_family = AF_INET;
    address.sin_port = htons(DNS_PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server, (struct sockaddr *)&address, sizeof(address)) != 0) {
        perror("bind DNS"); close(server); return EXIT_FAILURE;
    }
    signal(SIGTERM, stop_server);
    signal(SIGINT, stop_server);
    while (running) {
        client_size = sizeof(client);
        ssize_t size = recvfrom(server, request, sizeof(request), 0,
            (struct sockaddr *)&client, &client_size);
        if (size < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            perror("recvfrom DNS"); break;
        }
        if (size < 12 || request[2] & 0x80 || request[4] != 0 || request[5] != 1) continue;
        size_t end = question_end(request, (size_t)size);
        if (end == 0 || end + 16 > sizeof(response)) continue;
        memcpy(response, request, end);
        response[2] = 0x81;
        response[3] = 0x80;
        int is_ipv4 = request[end - 4] == 0 && request[end - 3] == 1 &&
            request[end - 2] == 0 && request[end - 1] == 1;
        response[6] = 0;
        response[7] = is_ipv4 ? 1 : 0;
        response[8] = response[9] = response[10] = response[11] = 0;
        size_t response_size = end;
        if (is_ipv4) {
            static const unsigned char answer[] = {
                0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
                0x00, 0x1e, 0x00, 0x04
            };
            memcpy(response + response_size, answer, sizeof(answer));
            response_size += sizeof(answer);
            memcpy(response + response_size, &portal.s_addr, 4);
            response_size += 4;
        }
        sendto(server, response, response_size, 0, (struct sockaddr *)&client, client_size);
    }
    close(server);
    return EXIT_SUCCESS;
}
