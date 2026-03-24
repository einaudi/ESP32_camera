#pragma once

#include "esp_err.h"

#define UART_PORT UART_NUM_0
#define UART_BAUD 115200

#define RX_BUF_SIZE 1024
#define CMD_BUF_SIZE 256
#define RESP_BUF_SIZE 128

#define MSG_FRAME_START  1
#define MSG_CHUNK        2
#define MSG_FRAME_END    3
#define MSG_LOG          4
#define MSG_TELEMETRY    5
#define CHUNK_SIZE 1024
#define SYNC_WORD 0x55AA55AA


typedef struct
{
    const char *path;

    esp_err_t (*set)(const char *arg);
    esp_err_t (*get)(char *out, size_t maxlen);

} command_entry_t;

typedef struct __attribute__((packed)) {
    uint32_t sync;      // 0x55AA
    uint8_t  type;      // message type
    uint8_t  seq;       // frame sequence
    uint16_t chunk_id;  // chunk number
    uint16_t length;    // payload length
} frame_header_t;

void uart_api_init(void);
void console_api_init(void);