#include "uart_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

//==================================================
// LOG
//==================================================

static const char *TAG = "UART_PROTOCOL";

//==================================================
// BUFFER DE RECEPCION
//==================================================

static char rx_buffer[UART_PROTOCOL_MAX_FRAME_LEN];

static size_t rx_index = 0;

static bool receiving_frame = false;

//==================================================
// CALLBACK
//==================================================

static uart_protocol_frame_callback_t frame_callback = NULL;

//==================================================
// DECLARACIONES INTERNAS
//==================================================

static uart_frame_type_t identify_frame_type(
    const char *type
);

static esp_err_t parse_frame(
    const char *frame_text,
    uart_protocol_frame_t *frame
);

//==================================================
// INICIALIZACION
//==================================================

void uart_protocol_init(void)
{
    memset(
        rx_buffer,
        0,
        sizeof(rx_buffer)
    );

    rx_index = 0;
    receiving_frame = false;

    ESP_LOGI(
        TAG,
        "MAIM Internal UART Protocol v1 inicializado"
    );
}

//==================================================
// CALLBACK
//==================================================

void uart_protocol_set_callback(
    uart_protocol_frame_callback_t callback
)
{
    frame_callback = callback;
}

//==================================================
// IDENTIFICAR TIPO
//==================================================

static uart_frame_type_t identify_frame_type(
    const char *type
)
{
    if (strcmp(type, "HELLO") == 0)
        return UART_FRAME_HELLO;

    if (strcmp(type, "CMD") == 0)
        return UART_FRAME_CMD;

    if (strcmp(type, "ACK") == 0)
        return UART_FRAME_ACK;

    if (strcmp(type, "DONE") == 0)
        return UART_FRAME_DONE;

    if (strcmp(type, "NACK") == 0)
        return UART_FRAME_NACK;

    if (strcmp(type, "STATE") == 0)
        return UART_FRAME_STATE;

    if (strcmp(type, "METRIC") == 0)
        return UART_FRAME_METRIC;

    if (strcmp(type, "SENSOR") == 0)
        return UART_FRAME_SENSOR;

    if (strcmp(type, "OUTPUT") == 0)
        return UART_FRAME_OUTPUT;

    if (strcmp(type, "EVENT") == 0)
        return UART_FRAME_EVENT;

    if (strcmp(type, "ERROR") == 0)
        return UART_FRAME_ERROR;

    if (strcmp(type, "ERROR_CLEAR") == 0)
        return UART_FRAME_ERROR_CLEAR;

    if (strcmp(type, "CONFIG") == 0)
        return UART_FRAME_CONFIG;

    if (strcmp(type, "SNAPSHOT") == 0)
        return UART_FRAME_SNAPSHOT;

    return UART_FRAME_UNKNOWN;
}

//==================================================
// PARSEAR TRAMA
//==================================================

static esp_err_t parse_frame(
    const char *frame_text,
    uart_protocol_frame_t *frame
)
{
    if (
        frame_text == NULL ||
        frame == NULL
    )
    {
        return ESP_ERR_INVALID_ARG;
    }

    size_t len = strlen(frame_text);

    //--------------------------------------------------
    // LONGITUD MINIMA
    //--------------------------------------------------

    if (len < 3)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    //--------------------------------------------------
    // VALIDAR DELIMITADORES
    //--------------------------------------------------

    if (
        frame_text[0] != '<' ||
        frame_text[len - 1] != '>'
    )
    {
        return ESP_ERR_INVALID_ARG;
    }

    //--------------------------------------------------
    // COPIA TEMPORAL SIN < >
    //--------------------------------------------------

    char temp[UART_PROTOCOL_MAX_FRAME_LEN];

    size_t content_len = len - 2;

    if (
        content_len >= sizeof(temp)
    )
    {
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(
        temp,
        &frame_text[1],
        content_len
    );

    temp[content_len] = '\0';

    //--------------------------------------------------
    // LIMPIAR RESULTADO
    //--------------------------------------------------

    memset(
        frame,
        0,
        sizeof(*frame)
    );

    //--------------------------------------------------
    // TOKENIZAR
    //--------------------------------------------------

    char *saveptr = NULL;

    char *token =
        strtok_r(
            temp,
            ",",
            &saveptr
        );

    if (token == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    //--------------------------------------------------
    // TIPO
    //--------------------------------------------------

    frame->type =
        identify_frame_type(token);

    if (
        frame->type ==
        UART_FRAME_UNKNOWN
    )
    {
        return ESP_ERR_NOT_SUPPORTED;
    }

    //--------------------------------------------------
    // CAMPOS
    //--------------------------------------------------

    uint8_t field_index = 0;

    while (
        (token = strtok_r(
            NULL,
            ",",
            &saveptr
        )) != NULL
    )
    {
        if (
            field_index >=
            UART_PROTOCOL_MAX_FIELDS
        )
        {
            return ESP_ERR_INVALID_SIZE;
        }

        if (
            strlen(token) >=
            UART_PROTOCOL_MAX_FIELD_LEN
        )
        {
            return ESP_ERR_INVALID_SIZE;
        }

        strncpy(
            frame->fields[field_index],
            token,
            UART_PROTOCOL_MAX_FIELD_LEN - 1
        );

        frame->fields[field_index]
                     [UART_PROTOCOL_MAX_FIELD_LEN - 1]
            = '\0';

        field_index++;
    }

    frame->field_count =
        field_index;

    return ESP_OK;
}

//==================================================
// PROCESAR TRAMA COMPLETA
//==================================================

esp_err_t uart_protocol_process_frame(
    const char *frame_text
)
{
    uart_protocol_frame_t frame;

    esp_err_t err =
        parse_frame(
            frame_text,
            &frame
        );

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "Trama invalida: %s | error=%s",
            frame_text,
            esp_err_to_name(err)
        );

        return err;
    }

    //--------------------------------------------------
    // LOG
    //--------------------------------------------------

    ESP_LOGI(
        TAG,
        "Trama valida | Tipo=%s | Campos=%u",
        uart_protocol_type_to_string(
            frame.type
        ),
        frame.field_count
    );

    for (
        uint8_t i = 0;
        i < frame.field_count;
        i++
    )
    {
        ESP_LOGI(
            TAG,
            "  Campo[%u] = %s",
            i,
            frame.fields[i]
        );
    }

    //--------------------------------------------------
    // CALLBACK
    //--------------------------------------------------

    if (frame_callback != NULL)
    {
        frame_callback(
            &frame
        );
    }

    return ESP_OK;
}

//==================================================
// PROCESAR CARACTER
//==================================================

void uart_protocol_process_char(char c)
{
    //--------------------------------------------------
    // INICIO DE TRAMA
    //--------------------------------------------------

    if (c == '<')
    {
        receiving_frame = true;
        rx_index = 0;

        rx_buffer[rx_index++] = c;

        return;
    }

    //--------------------------------------------------
    // IGNORAR SI NO ESTAMOS DENTRO DE TRAMA
    //--------------------------------------------------

    if (!receiving_frame)
    {
        return;
    }

    //--------------------------------------------------
    // OVERFLOW
    //--------------------------------------------------

    if (
        rx_index >=
        UART_PROTOCOL_MAX_FRAME_LEN - 1
    )
    {
        ESP_LOGW(
            TAG,
            "Trama descartada por longitud excesiva"
        );

        receiving_frame = false;
        rx_index = 0;

        return;
    }

    //--------------------------------------------------
    // GUARDAR CARACTER
    //--------------------------------------------------

    rx_buffer[rx_index++] = c;

    //--------------------------------------------------
    // FIN DE TRAMA
    //--------------------------------------------------

    if (c == '>')
    {
        rx_buffer[rx_index] =
            '\0';

        uart_protocol_process_frame(
            rx_buffer
        );

        receiving_frame = false;
        rx_index = 0;
    }
}

//==================================================
// GENERAR CMD
//==================================================

esp_err_t uart_protocol_build_command(
    char *buffer,
    size_t buffer_size,
    uint16_t transaction_id,
    const char *command,
    const char *params
)
{
    if (
        buffer == NULL ||
        buffer_size == 0 ||
        command == NULL
    )
    {
        return ESP_ERR_INVALID_ARG;
    }

    int written;

    if (
        params != NULL &&
        strlen(params) > 0
    )
    {
        written =
            snprintf(
                buffer,
                buffer_size,
                "<CMD,%u,%s,%s>",
                transaction_id,
                command,
                params
            );
    }
    else
    {
        written =
            snprintf(
                buffer,
                buffer_size,
                "<CMD,%u,%s>",
                transaction_id,
                command
            );
    }

    if (
        written < 0 ||
        written >= buffer_size
    )
    {
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

//==================================================
// TIPO → TEXTO
//==================================================

const char *uart_protocol_type_to_string(
    uart_frame_type_t type
)
{
    switch (type)
    {
        case UART_FRAME_HELLO:
            return "HELLO";

        case UART_FRAME_CMD:
            return "CMD";

        case UART_FRAME_ACK:
            return "ACK";

        case UART_FRAME_DONE:
            return "DONE";

        case UART_FRAME_NACK:
            return "NACK";

        case UART_FRAME_STATE:
            return "STATE";

        case UART_FRAME_METRIC:
            return "METRIC";

        case UART_FRAME_SENSOR:
            return "SENSOR";

        case UART_FRAME_OUTPUT:
            return "OUTPUT";

        case UART_FRAME_EVENT:
            return "EVENT";

        case UART_FRAME_ERROR:
            return "ERROR";

        case UART_FRAME_ERROR_CLEAR:
            return "ERROR_CLEAR";

        case UART_FRAME_CONFIG:
            return "CONFIG";

        case UART_FRAME_SNAPSHOT:
            return "SNAPSHOT";

        default:
            return "UNKNOWN";
    }
}