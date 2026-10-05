#ifndef AURABOT_PROTOCOL_H
#define AURABOT_PROTOCOL_H

#include <limits.h>

#ifndef AURABOT_RUNTIME_DIRECTORY
#define AURABOT_RUNTIME_DIRECTORY "/run/aurabot"
#endif
#ifndef AURABOT_SOCKET_PATH
#define AURABOT_SOCKET_PATH AURABOT_RUNTIME_DIRECTORY "/control.sock"
#endif
#define AURABOT_PROTOCOL_MAGIC 0x41555241U
#define AURABOT_PROTOCOL_VERSION 1U
#define AURABOT_REQUEST_HEADER_SIZE 20U
#define AURABOT_RESPONSE_HEADER_SIZE 24U
#define AURABOT_MAX_REQUEST_PAYLOAD 64U
#define AURABOT_MAX_RESPONSE_PAYLOAD 2048U
#define AURABOT_STATUS_FIELD_COUNT 14U
#define AURABOT_STATUS_PAYLOAD_SIZE (AURABOT_STATUS_FIELD_COUNT * 4U)

typedef enum {
    AURABOT_CMD_GET_STATUS = 1,
    AURABOT_CMD_SET_MODE,
    AURABOT_CMD_CLAIM_CONTROL,
    AURABOT_CMD_CONTROL_HEARTBEAT,
    AURABOT_CMD_RELEASE_CONTROL,
    AURABOT_CMD_DRIVE,
    AURABOT_CMD_STOP,
    AURABOT_CMD_EMERGENCY_STOP,
    AURABOT_CMD_GET_MAP,
    AURABOT_CMD_AUDIO_PLAY,
    AURABOT_CMD_AUDIO_PAUSE,
    AURABOT_CMD_AUDIO_STOP,
    AURABOT_CMD_AUDIO_SET_VOLUME,
    AURABOT_CMD_AUDIO_GET_TRACK_COUNT,
    AURABOT_CMD_AUDIO_GET_TRACK_NAME,
    AURABOT_CMD_DIGITAL_WRITE,
    AURABOT_CMD_PWM_SET,
    AURABOT_CMD_PWM_STOP
} aurabot_command_t;

_Static_assert(CHAR_BIT == 8, "AuraBot requiere chars de 8 bits");
_Static_assert(sizeof(int) == 4, "AuraBot requiere ints de 32 bits");
_Static_assert(sizeof(unsigned int) == 4,
               "AuraBot requiere unsigned ints de 32 bits");
_Static_assert(sizeof(long long) >= 8,
               "AuraBot requiere long long de al menos 64 bits");

static inline void aurabot_wire_put_uint(unsigned char *output,
                                         unsigned int value)
{
    output[0] = (unsigned char)(value & 0xffU);
    output[1] = (unsigned char)((value >> 8) & 0xffU);
    output[2] = (unsigned char)((value >> 16) & 0xffU);
    output[3] = (unsigned char)((value >> 24) & 0xffU);
}

static inline unsigned int aurabot_wire_get_uint(const unsigned char *input)
{
    return (unsigned int)input[0] |
           ((unsigned int)input[1] << 8) |
           ((unsigned int)input[2] << 16) |
           ((unsigned int)input[3] << 24);
}

static inline void aurabot_wire_put_int(unsigned char *output, int value)
{
    union {
        int signed_value;
        unsigned int unsigned_value;
    } bits;
    bits.signed_value = value;
    aurabot_wire_put_uint(output, bits.unsigned_value);
}

static inline int aurabot_wire_get_int(const unsigned char *input)
{
    union {
        int signed_value;
        unsigned int unsigned_value;
    } bits;
    bits.unsigned_value = aurabot_wire_get_uint(input);
    return bits.signed_value;
}

#endif
