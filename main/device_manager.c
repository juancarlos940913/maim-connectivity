#include "device_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

//==================================================
// LOG
//==================================================

static const char *TAG = "DEVICE_MANAGER";

//==================================================
// ALMACENAMIENTO
//==================================================

static device_value_entry_t states[DEVICE_MANAGER_MAX_STATES];

static device_value_entry_t metrics[DEVICE_MANAGER_MAX_METRICS];

static device_value_entry_t sensors[DEVICE_MANAGER_MAX_SENSORS];

static device_value_entry_t outputs[DEVICE_MANAGER_MAX_OUTPUTS];

static device_error_entry_t errors[DEVICE_MANAGER_MAX_ERRORS];

static device_controller_info_t controller_info;

//==================================================
// SNAPSHOT
//==================================================

static bool snapshot_active = false;
static uint16_t snapshot_transaction_id = 0;

//==================================================
// CALLBACKS
//==================================================

static device_manager_event_callback_t event_callback = NULL;

static device_manager_transaction_callback_t transaction_callback = NULL;

//==================================================
// UTILIDADES INTERNAS
//==================================================

static esp_err_t set_value(
    device_value_entry_t *table,
    size_t table_size,
    const char *key,
    const char *value);

static const char *get_value(
    const device_value_entry_t *table, size_t table_size, const char *key);

static esp_err_t add_error(const char *code, const char *severity);

static esp_err_t clear_error(const char *code);

static bool severity_is_valid(const char *severity);

static esp_err_t
parse_transaction_id(const char *text, uint16_t *transaction_id);

//==================================================
// INICIALIZACION
//==================================================

void device_manager_init(void)
{
    memset(states, 0, sizeof(states));

    memset(metrics, 0, sizeof(metrics));

    memset(sensors, 0, sizeof(sensors));

    memset(outputs, 0, sizeof(outputs));

    memset(errors, 0, sizeof(errors));

    memset(&controller_info, 0, sizeof(controller_info));

    snapshot_active = false;
    snapshot_transaction_id = 0;

    ESP_LOGI(TAG, "Device Manager inicializado");
}

//==================================================
// CALLBACKS
//==================================================

void device_manager_set_event_callback(device_manager_event_callback_t callback)
{
    event_callback = callback;
}

void device_manager_set_transaction_callback(
    device_manager_transaction_callback_t callback)
{
    transaction_callback = callback;
}

//==================================================
// GUARDAR KEY / VALUE
//==================================================

static esp_err_t set_value(
    device_value_entry_t *table,
    size_t table_size,
    const char *key,
    const char *value)
{
    if (table == NULL || key == NULL || value == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(key) >= DEVICE_MANAGER_KEY_LEN ||
        strlen(value) >= DEVICE_MANAGER_VALUE_LEN)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    //--------------------------------------------------
    // ACTUALIZAR EXISTENTE
    //--------------------------------------------------

    for (size_t i = 0; i < table_size; i++)
    {
        if (table[i].used && strcmp(table[i].key, key) == 0)
        {
            strncpy(table[i].value, value, DEVICE_MANAGER_VALUE_LEN - 1);

            table[i].value[DEVICE_MANAGER_VALUE_LEN - 1] = '\0';

            return ESP_OK;
        }
    }

    //--------------------------------------------------
    // CREAR NUEVA ENTRADA
    //--------------------------------------------------

    for (size_t i = 0; i < table_size; i++)
    {
        if (!table[i].used)
        {
            table[i].used = true;

            strncpy(table[i].key, key, DEVICE_MANAGER_KEY_LEN - 1);

            table[i].key[DEVICE_MANAGER_KEY_LEN - 1] = '\0';

            strncpy(table[i].value, value, DEVICE_MANAGER_VALUE_LEN - 1);

            table[i].value[DEVICE_MANAGER_VALUE_LEN - 1] = '\0';

            return ESP_OK;
        }
    }

    return ESP_ERR_NO_MEM;
}

//==================================================
// OBTENER KEY / VALUE
//==================================================

static const char *
get_value(const device_value_entry_t *table, size_t table_size, const char *key)
{
    if (table == NULL || key == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < table_size; i++)
    {
        if (table[i].used && strcmp(table[i].key, key) == 0)
        {
            return table[i].value;
        }
    }

    return NULL;
}

//==================================================
// SEVERIDAD VALIDA
//==================================================

static bool severity_is_valid(const char *severity)
{
    if (severity == NULL)
    {
        return false;
    }

    return strcmp(severity, "INFO") == 0 || strcmp(severity, "WARNING") == 0 ||
           strcmp(severity, "ERROR") == 0 || strcmp(severity, "CRITICAL") == 0;
}

//==================================================
// AGREGAR / ACTUALIZAR ERROR
//==================================================

static esp_err_t add_error(const char *code, const char *severity)
{
    if (code == NULL || severity == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!severity_is_valid(severity))
    {
        return ESP_ERR_INVALID_ARG;
    }

    //--------------------------------------------------
    // YA EXISTE
    //--------------------------------------------------

    for (size_t i = 0; i < DEVICE_MANAGER_MAX_ERRORS; i++)
    {
        if (errors[i].used && strcmp(errors[i].code, code) == 0)
        {
            strncpy(
                errors[i].severity, severity, sizeof(errors[i].severity) - 1);

            errors[i].severity[sizeof(errors[i].severity) - 1] = '\0';

            return ESP_OK;
        }
    }

    //--------------------------------------------------
    // NUEVO ERROR
    //--------------------------------------------------

    for (size_t i = 0; i < DEVICE_MANAGER_MAX_ERRORS; i++)
    {
        if (!errors[i].used)
        {
            errors[i].used = true;

            strncpy(errors[i].code, code, sizeof(errors[i].code) - 1);

            errors[i].code[sizeof(errors[i].code) - 1] = '\0';

            strncpy(
                errors[i].severity, severity, sizeof(errors[i].severity) - 1);

            errors[i].severity[sizeof(errors[i].severity) - 1] = '\0';

            return ESP_OK;
        }
    }

    return ESP_ERR_NO_MEM;
}

//==================================================
// ELIMINAR ERROR
//==================================================

static esp_err_t clear_error(const char *code)
{
    if (code == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < DEVICE_MANAGER_MAX_ERRORS; i++)
    {
        if (errors[i].used && strcmp(errors[i].code, code) == 0)
        {
            memset(&errors[i], 0, sizeof(errors[i]));

            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}

//==================================================
// ID DE TRANSACCION
//==================================================

static esp_err_t
parse_transaction_id(const char *text, uint16_t *transaction_id)
{
    if (text == NULL || transaction_id == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char *endptr = NULL;

    unsigned long value = strtoul(text, &endptr, 10);

    if (endptr == text || *endptr != '\0' || value > 65535)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *transaction_id = (uint16_t)value;

    return ESP_OK;
}

//==================================================
// PROCESAR TRAMA
//==================================================

esp_err_t device_manager_process_frame(const uart_protocol_frame_t *frame)
{
    if (frame == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;

    switch (frame->type)
    {
        //==================================================
        // HELLO
        //
        // <HELLO,MCU,MODEL,HW_REV,FW>
        //==================================================

    case UART_FRAME_HELLO:
    {
        if (frame->field_count != 4)
        {
            ESP_LOGW(TAG, "HELLO invalido. Se esperaban 4 campos");

            return ESP_ERR_INVALID_ARG;
        }

        strncpy(
            controller_info.mcu,
            frame->fields[0],
            sizeof(controller_info.mcu) - 1);

        strncpy(
            controller_info.model,
            frame->fields[1],
            sizeof(controller_info.model) - 1);

        strncpy(
            controller_info.hw_rev,
            frame->fields[2],
            sizeof(controller_info.hw_rev) - 1);

        strncpy(
            controller_info.fw_version,
            frame->fields[3],
            sizeof(controller_info.fw_version) - 1);

        controller_info.valid = true;

        ESP_LOGI(
            TAG,
            "Controlador identificado | MCU=%s | Modelo=%s | HW=%s | FW=%s",
            controller_info.mcu,
            controller_info.model,
            controller_info.hw_rev,
            controller_info.fw_version);

        break;
    }

        //==================================================
        // STATE
        //
        // <STATE,KEY,VALUE>
        //==================================================

    case UART_FRAME_STATE:
    {
        if (frame->field_count != 2)
        {
            ESP_LOGW(TAG, "STATE invalido. Se esperaban KEY,VALUE");

            return ESP_ERR_INVALID_ARG;
        }

        err = set_value(
            states,
            DEVICE_MANAGER_MAX_STATES,
            frame->fields[0],
            frame->fields[1]);

        if (err == ESP_OK)
        {
            ESP_LOGI(
                TAG,
                "STATE actualizado | %s=%s",
                frame->fields[0],
                frame->fields[1]);
        }

        break;
    }

        //==================================================
        // METRIC
        //
        // <METRIC,KEY,VALUE>
        //==================================================

    case UART_FRAME_METRIC:
    {
        if (frame->field_count != 2)
        {
            ESP_LOGW(TAG, "METRIC invalido. Se esperaban KEY,VALUE");

            return ESP_ERR_INVALID_ARG;
        }

        err = set_value(
            metrics,
            DEVICE_MANAGER_MAX_METRICS,
            frame->fields[0],
            frame->fields[1]);

        if (err == ESP_OK)
        {
            ESP_LOGI(
                TAG,
                "METRIC actualizado | %s=%s",
                frame->fields[0],
                frame->fields[1]);
        }

        break;
    }

        //==================================================
        // SENSOR
        //
        // <SENSOR,KEY,VALUE>
        //==================================================

    case UART_FRAME_SENSOR:
    {
        if (frame->field_count != 2)
        {
            ESP_LOGW(TAG, "SENSOR invalido. Se esperaban KEY,VALUE");

            return ESP_ERR_INVALID_ARG;
        }

        err = set_value(
            sensors,
            DEVICE_MANAGER_MAX_SENSORS,
            frame->fields[0],
            frame->fields[1]);

        if (err == ESP_OK)
        {
            ESP_LOGI(
                TAG,
                "SENSOR actualizado | %s=%s",
                frame->fields[0],
                frame->fields[1]);
        }

        break;
    }

        //==================================================
        // OUTPUT
        //
        // <OUTPUT,KEY,VALUE>
        //==================================================

    case UART_FRAME_OUTPUT:
    {
        if (frame->field_count != 2)
        {
            ESP_LOGW(TAG, "OUTPUT invalido. Se esperaban KEY,VALUE");

            return ESP_ERR_INVALID_ARG;
        }

        err = set_value(
            outputs,
            DEVICE_MANAGER_MAX_OUTPUTS,
            frame->fields[0],
            frame->fields[1]);

        if (err == ESP_OK)
        {
            ESP_LOGI(
                TAG,
                "OUTPUT actualizado | %s=%s",
                frame->fields[0],
                frame->fields[1]);
        }

        break;
    }

        //==================================================
        // ERROR
        //
        // <ERROR,CODE,SEVERITY>
        //==================================================

    case UART_FRAME_ERROR:
    {
        if (frame->field_count != 2)
        {
            ESP_LOGW(TAG, "ERROR invalido. Se esperaban CODE,SEVERITY");

            return ESP_ERR_INVALID_ARG;
        }

        err = add_error(frame->fields[0], frame->fields[1]);

        if (err == ESP_OK)
        {
            ESP_LOGW(
                TAG,
                "Error activo | %s | %s",
                frame->fields[0],
                frame->fields[1]);
        }
        else
        {
            ESP_LOGW(TAG, "Severidad o error invalido");
        }

        break;
    }

        //==================================================
        // ERROR CLEAR
        //
        // <ERROR_CLEAR,CODE>
        //==================================================

    case UART_FRAME_ERROR_CLEAR:
    {
        if (frame->field_count != 1)
        {
            ESP_LOGW(TAG, "ERROR_CLEAR invalido");

            return ESP_ERR_INVALID_ARG;
        }

        err = clear_error(frame->fields[0]);

        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Error eliminado | %s", frame->fields[0]);
        }
        else if (err == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGW(
                TAG,
                "ERROR_CLEAR recibido para error no activo: %s",
                frame->fields[0]);

            /*
             * No lo consideramos fallo grave.
             * El estado final ya es el esperado:
             * error no activo.
             */
            err = ESP_OK;
        }

        break;
    }

        //==================================================
        // EVENT
        //
        // <EVENT,TYPE,...>
        //==================================================

    case UART_FRAME_EVENT:
    {
        if (frame->field_count < 1)
        {
            ESP_LOGW(TAG, "EVENT sin tipo");

            return ESP_ERR_INVALID_ARG;
        }

        ESP_LOGI(TAG, "EVENT recibido | %s", frame->fields[0]);

        if (event_callback != NULL)
        {
            event_callback(frame->fields[0], frame);
        }

        break;
    }

        //==================================================
        // SNAPSHOT
        //
        // <SNAPSHOT,BEGIN,ID>
        // <SNAPSHOT,END,ID>
        //==================================================

    case UART_FRAME_SNAPSHOT:
    {
        if (frame->field_count != 2)
        {
            ESP_LOGW(TAG, "SNAPSHOT invalido");

            return ESP_ERR_INVALID_ARG;
        }

        uint16_t transaction_id;

        err = parse_transaction_id(frame->fields[1], &transaction_id);

        if (err != ESP_OK)
        {
            ESP_LOGW(TAG, "ID de SNAPSHOT invalido");

            return err;
        }

        //--------------------------------------------------
        // BEGIN
        //--------------------------------------------------

        if (strcmp(frame->fields[0], "BEGIN") == 0)
        {
            snapshot_active = true;

            snapshot_transaction_id = transaction_id;

            ESP_LOGI(TAG, "SNAPSHOT BEGIN | ID=%u", transaction_id);
        }

        //--------------------------------------------------
        // END
        //--------------------------------------------------

        else if (strcmp(frame->fields[0], "END") == 0)
        {
            if (!snapshot_active || transaction_id != snapshot_transaction_id)
            {
                ESP_LOGW(TAG, "SNAPSHOT END no corresponde al activo");

                return ESP_ERR_INVALID_STATE;
            }

            snapshot_active = false;

            ESP_LOGI(TAG, "SNAPSHOT END | ID=%u", transaction_id);
        }

        //--------------------------------------------------
        // OPERACION DESCONOCIDA
        //--------------------------------------------------

        else
        {
            ESP_LOGW(
                TAG, "Operacion SNAPSHOT desconocida: %s", frame->fields[0]);

            return ESP_ERR_NOT_SUPPORTED;
        }

        break;
    }

        //==================================================
        // ACK / DONE / NACK
        //==================================================

    case UART_FRAME_ACK:
    case UART_FRAME_DONE:
    case UART_FRAME_NACK:
    {
        if (frame->field_count < 1)
        {
            ESP_LOGW(TAG, "Respuesta de transaccion sin ID");

            return ESP_ERR_INVALID_ARG;
        }

        uint16_t transaction_id;

        err = parse_transaction_id(frame->fields[0], &transaction_id);

        if (err != ESP_OK)
        {
            ESP_LOGW(TAG, "ID de transaccion invalido");

            return err;
        }

        ESP_LOGI(
            TAG,
            "Transaccion | %s | ID=%u",
            uart_protocol_type_to_string(frame->type),
            transaction_id);

        if (transaction_callback != NULL)
        {
            transaction_callback(frame->type, transaction_id, frame);
        }

        break;
    }

        //==================================================
        // CONFIG
        //
        // Por ahora se valida y se informa.
        // Posteriormente tendrá almacenamiento separado.
        //==================================================

    case UART_FRAME_CONFIG:
    {
        if (frame->field_count != 2)
        {
            ESP_LOGW(TAG, "CONFIG invalido. Se esperaban KEY,VALUE");

            return ESP_ERR_INVALID_ARG;
        }

        ESP_LOGI(
            TAG, "CONFIG recibido | %s=%s", frame->fields[0], frame->fields[1]);

        break;
    }

        //==================================================
        // CMD
        //
        // El ESP32 normalmente GENERARA CMD, no debería
        // recibirlo desde el controlador.
        //==================================================

    case UART_FRAME_CMD:
    {
        ESP_LOGW(TAG, "CMD recibido desde controlador. Ignorado");

        return ESP_ERR_NOT_SUPPORTED;
    }

    default:
    {
        ESP_LOGW(
            TAG,
            "Tipo de trama no gestionado: %s",
            uart_protocol_type_to_string(frame->type));

        return ESP_ERR_NOT_SUPPORTED;
    }
    }

    //--------------------------------------------------
    // ERROR FINAL
    //--------------------------------------------------

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Error procesando %s: %s",
            uart_protocol_type_to_string(frame->type),
            esp_err_to_name(err));
    }

    return err;
}

//==================================================
// SNAPSHOT
//==================================================

bool device_manager_snapshot_active(void)
{
    return snapshot_active;
}

uint16_t device_manager_snapshot_id(void)
{
    return snapshot_transaction_id;
}

//==================================================
// IDENTIDAD
//==================================================

const device_controller_info_t *device_manager_get_controller_info(void)
{
    return &controller_info;
}

//==================================================
// GETTERS STRING
//==================================================

const char *device_manager_get_state(const char *key)
{
    return get_value(states, DEVICE_MANAGER_MAX_STATES, key);
}

const char *device_manager_get_metric(const char *key)
{
    return get_value(metrics, DEVICE_MANAGER_MAX_METRICS, key);
}

const char *device_manager_get_sensor(const char *key)
{
    return get_value(sensors, DEVICE_MANAGER_MAX_SENSORS, key);
}

const char *device_manager_get_output(const char *key)
{
    return get_value(outputs, DEVICE_MANAGER_MAX_OUTPUTS, key);
}

//==================================================
// METRIC -> INT64
//==================================================

esp_err_t device_manager_get_metric_int64(const char *key, int64_t *value)
{
    if (value == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *text = device_manager_get_metric(key);

    if (text == NULL)
    {
        return ESP_ERR_NOT_FOUND;
    }

    char *endptr = NULL;

    long long parsed = strtoll(text, &endptr, 10);

    if (endptr == text || *endptr != '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }

    *value = parsed;

    return ESP_OK;
}

//==================================================
// SENSOR -> FLOAT
//==================================================

esp_err_t device_manager_get_sensor_float(const char *key, float *value)
{
    if (value == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *text = device_manager_get_sensor(key);

    if (text == NULL)
    {
        return ESP_ERR_NOT_FOUND;
    }

    char *endptr = NULL;

    float parsed = strtof(text, &endptr);

    if (endptr == text || *endptr != '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }

    *value = parsed;

    return ESP_OK;
}

//==================================================
// OUTPUT -> BOOL
//==================================================

esp_err_t device_manager_get_output_bool(const char *key, bool *value)
{
    if (value == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *text = device_manager_get_output(key);

    if (text == NULL)
    {
        return ESP_ERR_NOT_FOUND;
    }

    if (strcmp(text, "1") == 0)
    {
        *value = true;
        return ESP_OK;
    }

    if (strcmp(text, "0") == 0)
    {
        *value = false;
        return ESP_OK;
    }

    return ESP_ERR_INVALID_ARG;
}

//==================================================
// ERRORES
//==================================================

size_t device_manager_get_error_count(void)
{
    size_t count = 0;

    for (size_t i = 0; i < DEVICE_MANAGER_MAX_ERRORS; i++)
    {
        if (errors[i].used)
        {
            count++;
        }
    }

    return count;
}

const device_error_entry_t *device_manager_get_error(size_t index)
{
    size_t current = 0;

    for (size_t i = 0; i < DEVICE_MANAGER_MAX_ERRORS; i++)
    {
        if (!errors[i].used)
        {
            continue;
        }

        if (current == index)
        {
            return &errors[i];
        }

        current++;
    }

    return NULL;
}

//==================================================
// DEBUG
//==================================================

static void print_table(
    const char *name, const device_value_entry_t *table, size_t table_size)
{
    ESP_LOGI(TAG, "--- %s ---", name);

    bool found = false;

    for (size_t i = 0; i < table_size; i++)
    {
        if (table[i].used)
        {
            ESP_LOGI(TAG, "%s = %s", table[i].key, table[i].value);

            found = true;
        }
    }

    if (!found)
    {
        ESP_LOGI(TAG, "(vacio)");
    }
}

void device_manager_print_status(void)
{
    ESP_LOGI(TAG, "================================");

    ESP_LOGI(TAG, "     DEVICE MANAGER STATUS");

    ESP_LOGI(TAG, "================================");

    //--------------------------------------------------
    // CONTROLADOR
    //--------------------------------------------------

    if (controller_info.valid)
    {
        ESP_LOGI(
            TAG,
            "Controller: %s | %s | HW %s | FW %s",
            controller_info.mcu,
            controller_info.model,
            controller_info.hw_rev,
            controller_info.fw_version);
    }
    else
    {
        ESP_LOGI(TAG, "Controller: no identificado");
    }

    //--------------------------------------------------
    // TABLAS
    //--------------------------------------------------

    print_table("STATE", states, DEVICE_MANAGER_MAX_STATES);

    print_table("METRICS", metrics, DEVICE_MANAGER_MAX_METRICS);

    print_table("SENSORS", sensors, DEVICE_MANAGER_MAX_SENSORS);

    print_table("OUTPUTS", outputs, DEVICE_MANAGER_MAX_OUTPUTS);

    //--------------------------------------------------
    // ERRORES
    //--------------------------------------------------

    ESP_LOGI(TAG, "--- ERRORS ---");

    size_t count = device_manager_get_error_count();

    if (count == 0)
    {
        ESP_LOGI(TAG, "(sin errores)");
    }
    else
    {
        for (size_t i = 0; i < count; i++)
        {
            const device_error_entry_t *error = device_manager_get_error(i);

            if (error != NULL)
            {
                ESP_LOGI(TAG, "%s | %s", error->code, error->severity);
            }
        }
    }

    ESP_LOGI(
        TAG,
        "Snapshot: %s | ID=%u",
        snapshot_active ? "ACTIVE" : "INACTIVE",
        snapshot_transaction_id);
}