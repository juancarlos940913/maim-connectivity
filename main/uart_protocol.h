#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

//==================================================
// CONFIGURACION DEL PROTOCOLO
//==================================================

#define UART_PROTOCOL_MAX_FRAME_LEN 128
#define UART_PROTOCOL_MAX_FIELDS 8
#define UART_PROTOCOL_MAX_FIELD_LEN 32

//==================================================
// TIPOS DE TRAMA
//==================================================

typedef enum
{
    UART_FRAME_UNKNOWN = 0,

    UART_FRAME_HELLO,
    UART_FRAME_CMD,
    UART_FRAME_ACK,
    UART_FRAME_DONE,
    UART_FRAME_NACK,

    UART_FRAME_STATE,
    UART_FRAME_METRIC,
    UART_FRAME_SENSOR,
    UART_FRAME_OUTPUT,

    UART_FRAME_EVENT,

    UART_FRAME_ERROR,
    UART_FRAME_ERROR_CLEAR,

    UART_FRAME_CONFIG,

    UART_FRAME_SNAPSHOT

} uart_frame_type_t;

//==================================================
// ESTRUCTURA DE TRAMA PARSEADA
//==================================================

typedef struct
{
    uart_frame_type_t type;

    uint8_t field_count;

    char fields[UART_PROTOCOL_MAX_FIELDS][UART_PROTOCOL_MAX_FIELD_LEN];

} uart_protocol_frame_t;

//==================================================
// CALLBACK
//==================================================

typedef void (*uart_protocol_frame_callback_t)(
    const uart_protocol_frame_t *frame);

//==================================================
// INICIALIZACION
//==================================================

void uart_protocol_init(void);

//==================================================
// CALLBACK
//==================================================

void uart_protocol_set_callback(uart_protocol_frame_callback_t callback);

//==================================================
// ENTRADA DE CARACTERES
//==================================================

void uart_protocol_process_char(char c);

//==================================================
// PROCESAR TRAMA COMPLETA DIRECTAMENTE
//
// Util para pruebas:
//
// uart_protocol_process_frame(
//     "<STATE,MODE,STANDBY>"
// );
//
//==================================================

esp_err_t uart_protocol_process_frame(const char *frame_text);

//==================================================
// GENERAR TRAMA CMD
//==================================================

esp_err_t uart_protocol_build_command(
    char *buffer,
    size_t buffer_size,
    uint16_t transaction_id,
    const char *command,
    const char *params);

//==================================================
// UTILIDADES
//==================================================

const char *uart_protocol_type_to_string(uart_frame_type_t type);