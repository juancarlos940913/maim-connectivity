#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "uart_protocol.h"

//==================================================
// CONFIGURACION
//==================================================

#define TRANSACTION_MANAGER_MAX_ACTIVE 8
#define TRANSACTION_MANAGER_COMMAND_ID_LEN 48
#define TRANSACTION_MANAGER_COMMAND_LEN 32

//--------------------------------------------------
// TIMEOUTS DEFAULT
//--------------------------------------------------

#define TRANSACTION_ACK_TIMEOUT_MS 5000
#define TRANSACTION_EXECUTION_TIMEOUT_MS 30000

//==================================================
// ESTADOS
//==================================================

typedef enum
{
    TRANSACTION_STATE_FREE = 0,

    TRANSACTION_STATE_WAITING_ACK,
    TRANSACTION_STATE_ACKED,
    TRANSACTION_STATE_COMPLETED,
    TRANSACTION_STATE_REJECTED,
    TRANSACTION_STATE_FAILED

} transaction_state_t;

//==================================================
// CALLBACK
//==================================================

typedef void (*transaction_manager_result_callback_t)(
    const char *mqtt_command_id, transaction_state_t state, const char *reason);

//==================================================
// INICIALIZACION
//==================================================

void transaction_manager_init(void);

//==================================================
// CALLBACK
//==================================================

void transaction_manager_set_result_callback(
    transaction_manager_result_callback_t callback);

//==================================================
// CREAR TRANSACCION
//==================================================

esp_err_t transaction_manager_create(
    const char *mqtt_command_id,
    const char *command,
    const char *params,
    uint16_t *uart_transaction_id);

//==================================================
// PROCESAR RESPUESTA UART
//==================================================

esp_err_t transaction_manager_process_uart_response(
    uart_frame_type_t type,
    uint16_t transaction_id,
    const uart_protocol_frame_t *frame);

//==================================================
// TIMEOUTS
//
// Llamar periodicamente.
//==================================================

void transaction_manager_process_timeouts(void);

//==================================================
// DEBUG
//==================================================

void transaction_manager_print_status(void);