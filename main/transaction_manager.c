#include "transaction_manager.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "uart_transport.h"

//==================================================
// LOG
//==================================================

static const char *TAG = "TRANSACTION_MANAGER";

//==================================================
// ESTRUCTURA INTERNA
//==================================================

typedef struct
{
    bool used;

    uint16_t uart_id;

    transaction_origin_t origin;

    char mqtt_command_id[TRANSACTION_MANAGER_COMMAND_ID_LEN];

    char command[TRANSACTION_MANAGER_COMMAND_LEN];

    transaction_state_t state;

    //--------------------------------------------------
    // CONTROL DE TIEMPOS
    //--------------------------------------------------

    int64_t created_us;
    int64_t ack_received_us;

} transaction_entry_t;

//==================================================
// VARIABLES
//==================================================

static transaction_entry_t transactions[TRANSACTION_MANAGER_MAX_ACTIVE];

static uint16_t next_uart_id = 1;

static transaction_manager_result_callback_t result_callback = NULL;

//==================================================
// UTILIDADES
//==================================================

static transaction_entry_t *find_transaction(uint16_t uart_id)
{
    for (size_t i = 0; i < TRANSACTION_MANAGER_MAX_ACTIVE; i++)
    {
        if (transactions[i].used && transactions[i].uart_id == uart_id)
        {
            return &transactions[i];
        }
    }

    return NULL;
}

static void release_transaction(transaction_entry_t *transaction)
{
    if (transaction == NULL)
    {
        return;
    }

    memset(transaction, 0, sizeof(*transaction));
}

//==================================================
// INICIALIZACION
//==================================================

void transaction_manager_init(void)
{
    memset(transactions, 0, sizeof(transactions));

    next_uart_id = 1;

    ESP_LOGI(TAG, "Transaction Manager inicializado");
}

//==================================================
// CALLBACK
//==================================================

void transaction_manager_set_result_callback(
    transaction_manager_result_callback_t callback)
{
    result_callback = callback;
}

//--------------------------------------------------
// TRANSACCION PRIVADA INTERNA
//--------------------------------------------------

static esp_err_t create_transaction(
    transaction_origin_t origin,
    const char *mqtt_command_id,
    const char *command,
    const char *params,
    uint16_t *uart_transaction_id)
{
    if (command == NULL || uart_transaction_id == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (origin == TRANSACTION_ORIGIN_MQTT && mqtt_command_id == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    //--------------------------------------------------
    // BUSCAR SLOT LIBRE
    //--------------------------------------------------

    transaction_entry_t *transaction = NULL;

    for (size_t i = 0; i < TRANSACTION_MANAGER_MAX_ACTIVE; i++)
    {
        if (!transactions[i].used)
        {
            transaction = &transactions[i];
            break;
        }
    }

    if (transaction == NULL)
    {
        ESP_LOGE(TAG, "No hay espacio para nueva transaccion");

        return ESP_ERR_NO_MEM;
    }

    //--------------------------------------------------
    // ASIGNAR ID
    //--------------------------------------------------

    uint16_t id = next_uart_id++;

    if (next_uart_id == 0)
    {
        next_uart_id = 1;
    }

    //--------------------------------------------------
    // GUARDAR
    //--------------------------------------------------

    transaction->used = true;
    transaction->uart_id = id;
    transaction->origin = origin;

    if (mqtt_command_id != NULL)
    {
        strncpy(
            transaction->mqtt_command_id,
            mqtt_command_id,
            sizeof(transaction->mqtt_command_id) - 1);
    }

    strncpy(transaction->command, command, sizeof(transaction->command) - 1);

    transaction->state = TRANSACTION_STATE_WAITING_ACK;
    transaction->created_us = esp_timer_get_time();
    transaction->ack_received_us = 0;

    *uart_transaction_id = id;

    //--------------------------------------------------
    // GENERAR TRAMA UART
    //--------------------------------------------------

    char uart_frame[UART_PROTOCOL_MAX_FRAME_LEN];

    esp_err_t err = uart_protocol_build_command(
        uart_frame, sizeof(uart_frame), id, command, params);

    if (err != ESP_OK)
    {
        release_transaction(transaction);

        return err;
    }

    //--------------------------------------------------
    // ENVIAR AL CONTROLADOR
    //--------------------------------------------------

    err = uart_transport_send(uart_frame, strlen(uart_frame));

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Error enviando comando UART | ID=%u | CMD=%s | Error=%s",
            id,
            command,
            esp_err_to_name(err));

        release_transaction(transaction);

        return err;
    }

    //--------------------------------------------------
    // LOG
    //--------------------------------------------------

    if (origin == TRANSACTION_ORIGIN_MQTT)
    {
        ESP_LOGI(
            TAG,
            "Comando enviado | UART ID=%u | MQTT ID=%s | CMD=%s",
            id,
            mqtt_command_id,
            command);
    }
    else
    {
        ESP_LOGI(
            TAG, "Comando interno enviado | UART ID=%u | CMD=%s", id, command);
    }

    ESP_LOGI(TAG, "Esperando ACK del controlador | UART ID=%u", id);

    return ESP_OK;
}

//==================================================
// CREAR TRANSACCION
//==================================================

esp_err_t transaction_manager_create(
    const char *mqtt_command_id,
    const char *command,
    const char *params,
    uint16_t *uart_transaction_id)
{
    return create_transaction(
        TRANSACTION_ORIGIN_MQTT,
        mqtt_command_id,
        command,
        params,
        uart_transaction_id);
}

esp_err_t transaction_manager_create_internal(
    const char *command, const char *params, uint16_t *uart_transaction_id)
{
    return create_transaction(
        TRANSACTION_ORIGIN_INTERNAL,
        NULL,
        command,
        params,
        uart_transaction_id);
}

//==================================================
// PROCESAR RESPUESTA UART
//==================================================

esp_err_t transaction_manager_process_uart_response(
    uart_frame_type_t type,
    uint16_t transaction_id,
    const uart_protocol_frame_t *frame)
{
    transaction_entry_t *transaction = find_transaction(transaction_id);

    if (transaction == NULL)
    {
        ESP_LOGW(
            TAG,
            "Respuesta para transaccion desconocida ID=%u",
            transaction_id);

        return ESP_ERR_NOT_FOUND;
    }

    //--------------------------------------------------
    // ACK
    //--------------------------------------------------

    if (type == UART_FRAME_ACK)
    {
        //--------------------------------------------------
        // ACK DUPLICADO
        //--------------------------------------------------

        if (transaction->state == TRANSACTION_STATE_ACKED)
        {
            ESP_LOGW(
                TAG, "ACK duplicado ignorado | UART ID=%u", transaction_id);

            return ESP_OK;
        }

        //--------------------------------------------------
        // ACK FUERA DE ESTADO
        //--------------------------------------------------

        if (transaction->state != TRANSACTION_STATE_WAITING_ACK)
        {
            ESP_LOGW(
                TAG,
                "ACK inesperado | UART ID=%u | Estado=%d",
                transaction_id,
                transaction->state);

            return ESP_ERR_INVALID_STATE;
        }

        //--------------------------------------------------
        // REGISTRAR ACK
        //--------------------------------------------------

        transaction->state = TRANSACTION_STATE_ACKED;

        transaction->ack_received_us = esp_timer_get_time();

        ESP_LOGI(
            TAG,
            "ACK recibido | UART ID=%u | MQTT ID=%s",
            transaction_id,
            transaction->mqtt_command_id);

        if (result_callback != NULL)
        {
            result_callback(
                transaction->origin,
                transaction->mqtt_command_id,
                transaction->command,
                TRANSACTION_STATE_ACKED,
                NULL);
        }

        return ESP_OK;
    }

    //--------------------------------------------------
    // DONE
    //--------------------------------------------------

    if (type == UART_FRAME_DONE)
    {
        transaction->state = TRANSACTION_STATE_COMPLETED;

        ESP_LOGI(
            TAG,
            "DONE recibido | UART ID=%u | MQTT ID=%s",
            transaction_id,
            transaction->mqtt_command_id);

        if (result_callback != NULL)
        {
            result_callback(
                transaction->origin,
                transaction->mqtt_command_id,
                transaction->command,
                TRANSACTION_STATE_COMPLETED,
                NULL);
        }

        release_transaction(transaction);

        return ESP_OK;
    }

    //--------------------------------------------------
    // NACK
    //--------------------------------------------------

    if (type == UART_FRAME_NACK)
    {
        transaction->state = TRANSACTION_STATE_REJECTED;

        const char *reason = "UNKNOWN";

        if (frame != NULL && frame->field_count >= 2)
        {
            reason = frame->fields[1];
        }

        ESP_LOGW(
            TAG,
            "NACK recibido | UART ID=%u | MQTT ID=%s | Reason=%s",
            transaction_id,
            transaction->mqtt_command_id,
            reason);

        if (result_callback != NULL)
        {
            result_callback(
                transaction->origin,
                transaction->mqtt_command_id,
                transaction->command,
                TRANSACTION_STATE_REJECTED,
                reason);
        }

        release_transaction(transaction);

        return ESP_OK;
    }

    return ESP_ERR_NOT_SUPPORTED;
}

//==================================================
// PROCESAR TIMEOUTS
//==================================================

void transaction_manager_process_timeouts(void)
{
    int64_t now_us = esp_timer_get_time();

    //--------------------------------------------------
    // Convertimos los timeout a microsegundos.
    //--------------------------------------------------

    const int64_t ack_timeout_us = (int64_t)TRANSACTION_ACK_TIMEOUT_MS * 1000LL;

    const int64_t execution_timeout_us =
        (int64_t)TRANSACTION_EXECUTION_TIMEOUT_MS * 1000LL;

    //--------------------------------------------------
    // REVISAR TODAS LAS TRANSACCIONES
    //--------------------------------------------------

    for (size_t i = 0; i < TRANSACTION_MANAGER_MAX_ACTIVE; i++)
    {
        transaction_entry_t *transaction = &transactions[i];

        if (!transaction->used)
        {
            continue;
        }

        //==================================================
        // ESPERANDO ACK
        //==================================================

        if (transaction->state == TRANSACTION_STATE_WAITING_ACK)
        {
            int64_t elapsed_us = now_us - transaction->created_us;

            if (elapsed_us >= ack_timeout_us)
            {
                ESP_LOGE(
                    TAG,
                    "ACK TIMEOUT | UART ID=%u | MQTT ID=%s | CMD=%s",
                    transaction->uart_id,
                    transaction->mqtt_command_id,
                    transaction->command);

                //--------------------------------------------------
                // AVISAR AL MQTT
                //--------------------------------------------------

                if (result_callback != NULL)
                {
                    result_callback(
                        transaction->origin,
                        transaction->mqtt_command_id,
                        transaction->command,
                        TRANSACTION_STATE_FAILED,
                        "ACK_TIMEOUT");
                }

                //--------------------------------------------------
                // LIBERAR
                //--------------------------------------------------

                release_transaction(transaction);

                continue;
            }
        }

        //==================================================
        // ACK RECIBIDO, ESPERANDO DONE
        //==================================================

        else if (transaction->state == TRANSACTION_STATE_ACKED)
        {
            int64_t elapsed_us = now_us - transaction->ack_received_us;

            if (elapsed_us >= execution_timeout_us)
            {
                ESP_LOGE(
                    TAG,
                    "EXECUTION TIMEOUT | UART ID=%u | MQTT ID=%s | CMD=%s",
                    transaction->uart_id,
                    transaction->mqtt_command_id,
                    transaction->command);

                //--------------------------------------------------
                // AVISAR AL MQTT
                //--------------------------------------------------

                if (result_callback != NULL)
                {
                    result_callback(
                        transaction->origin,
                        transaction->mqtt_command_id,
                        transaction->command,
                        TRANSACTION_STATE_FAILED,
                        "EXECUTION_TIMEOUT");
                }

                //--------------------------------------------------
                // LIBERAR
                //--------------------------------------------------

                release_transaction(transaction);

                continue;
            }
        }
    }
}

//==================================================
// DEBUG
//==================================================

void transaction_manager_print_status(void)
{
    ESP_LOGI(TAG, "------ TRANSACCIONES ------");

    bool found = false;

    for (size_t i = 0; i < TRANSACTION_MANAGER_MAX_ACTIVE; i++)
    {
        if (!transactions[i].used)
        {
            continue;
        }

        ESP_LOGI(
            TAG,
            "UART=%u | MQTT=%s | CMD=%s | STATE=%d",
            transactions[i].uart_id,
            transactions[i].mqtt_command_id,
            transactions[i].command,
            transactions[i].state);

        found = true;
    }

    if (!found)
    {
        ESP_LOGI(TAG, "(sin transacciones activas)");
    }
}