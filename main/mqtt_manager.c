#include "mqtt_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mqtt_client.h"
#include "esp_log.h"

#include "cJSON.h"

#include "maim_config.h"
#include "wifi_manager.h"
#include "time_manager.h"
#include "device_manager.h"
#include "transaction_manager.h"

//==================================================
// LOG
//==================================================

static const char *TAG = "MQTT_MANAGER";

//==================================================
// CLIENTE MQTT
//==================================================

static esp_mqtt_client_handle_t mqtt_client = NULL;

static bool mqtt_connected = false;

static uint32_t event_sequence = 0;

static void generate_event_id(char *buffer, size_t buffer_size)
{
    event_sequence++;

    snprintf(
        buffer,
        buffer_size,
        "EVT-%s-%06lu",
        MAIM_DEVICE_ID,
        (unsigned long)event_sequence);
}

//==================================================
// TOPICS
//==================================================

static char topic_availability[128];
static char topic_state_reported[128];
static char topic_telemetry[128];
static char topic_command_request[128];
static char topic_command_response[128];
static char topic_event[128];

//==================================================
// CONSTRUIR TOPICS
//==================================================

static void build_topics(void)
{
    snprintf(
        topic_availability,
        sizeof(topic_availability),
        "%s/%s/availability",
        MAIM_MQTT_ROOT,
        MAIM_DEVICE_ID);

    snprintf(
        topic_state_reported,
        sizeof(topic_state_reported),
        "%s/%s/state/reported",
        MAIM_MQTT_ROOT,
        MAIM_DEVICE_ID);

    snprintf(
        topic_telemetry,
        sizeof(topic_telemetry),
        "%s/%s/telemetry",
        MAIM_MQTT_ROOT,
        MAIM_DEVICE_ID);

    snprintf(
        topic_command_request,
        sizeof(topic_command_request),
        "%s/%s/command/request",
        MAIM_MQTT_ROOT,
        MAIM_DEVICE_ID);

    snprintf(
        topic_command_response,
        sizeof(topic_command_response),
        "%s/%s/command/response",
        MAIM_MQTT_ROOT,
        MAIM_DEVICE_ID);

    snprintf(
        topic_event,
        sizeof(topic_event),
        "%s/%s/event",
        MAIM_MQTT_ROOT,
        MAIM_DEVICE_ID);
}

//==================================================
// PUBLICAR JSON
//==================================================

static esp_err_t
publish_json(const char *topic, cJSON *root, int qos, bool retain)
{
    if (!mqtt_connected || mqtt_client == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    char *payload = cJSON_PrintUnformatted(root);

    if (payload == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    int msg_id =
        esp_mqtt_client_publish(mqtt_client, topic, payload, 0, qos, retain);

    if (msg_id < 0)
    {
        ESP_LOGE(TAG, "Error publicando en %s", topic);

        cJSON_free(payload);

        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Publicado [%s] %s", topic, payload);

    cJSON_free(payload);

    return ESP_OK;
}

//==================================================
// RESPUESTA DE COMANDO
//==================================================

static void publish_command_response(
    const char *command_id, const char *status, const char *response)
{
    cJSON *root = cJSON_CreateObject();

    if (root == NULL)
    {
        return;
    }

    cJSON_AddNumberToObject(root, "schema", 1);

    cJSON_AddStringToObject(root, "command_id", command_id);

    cJSON_AddNumberToObject(root, "ts", time_manager_get_timestamp());

    cJSON_AddStringToObject(root, "status", status);

    //--------------------------------------------------
    // RESULT
    //--------------------------------------------------

    if (response != NULL)
    {
        cJSON *result = cJSON_AddObjectToObject(root, "result");

        if (result != NULL)
        {
            cJSON_AddStringToObject(result, "response", response);
        }
    }

    publish_json(topic_command_response, root, 1, false);

    cJSON_Delete(root);
}

//==================================================
// PROCESAR COMMAND/REQUEST
//==================================================

static void process_command(const char *payload, int payload_len)
{
    //--------------------------------------------------
    // MQTT no garantiza terminador NULL.
    //--------------------------------------------------

    char *buffer = calloc(payload_len + 1, sizeof(char));

    if (buffer == NULL)
    {
        ESP_LOGE(TAG, "Sin memoria para comando MQTT");

        return;
    }

    memcpy(buffer, payload, payload_len);

    buffer[payload_len] = '\0';

    ESP_LOGI(TAG, "Comando recibido: %s", buffer);

    //--------------------------------------------------
    // PARSEAR JSON
    //--------------------------------------------------

    cJSON *root = cJSON_Parse(buffer);

    free(buffer);

    if (root == NULL)
    {
        ESP_LOGE(TAG, "JSON de comando invalido");

        return;
    }

    //--------------------------------------------------
    // command_id
    //--------------------------------------------------

    cJSON *command_id_json =
        cJSON_GetObjectItemCaseSensitive(root, "command_id");

    //--------------------------------------------------
    // command
    //--------------------------------------------------

    cJSON *command_json = cJSON_GetObjectItemCaseSensitive(root, "command");

    if (!cJSON_IsString(command_id_json) || !cJSON_IsString(command_json))
    {
        ESP_LOGE(TAG, "Comando sin command_id o command valido");

        cJSON_Delete(root);
        return;
    }

    const char *command_id = command_id_json->valuestring;

    const char *command = command_json->valuestring;

    //--------------------------------------------------
    // PING
    //--------------------------------------------------

    if (strcmp(command, "PING") == 0)
    {
        ESP_LOGI(TAG, "PING recibido");

        publish_command_response(command_id, "SUCCESS", "PONG");
    }

    //--------------------------------------------------
    // GET_STATE
    //--------------------------------------------------

    else if (strcmp(command, "GET_STATE") == 0)
    {
        uint16_t uart_id;

        esp_err_t err =
            transaction_manager_create(command_id, "GET_STATE", NULL, &uart_id);

        if (err != ESP_OK)
        {
            publish_command_response(
                command_id, "FAILED", "UART_TRANSACTION_ERROR");

            cJSON_Delete(root);
            return;
        }

        ESP_LOGI(TAG, "GET_STATE enviado a controlador | UART ID=%u", uart_id);

        /*
         * No publicamos SUCCESS aqui.
         * Esperamos ACK / SNAPSHOT / DONE del controlador.
         */
    }

    else if (strcmp(command, "DISPENSE") == 0)
    {
        uint16_t uart_id;

        esp_err_t err =
            transaction_manager_create(command_id, "DISPENSE", NULL, &uart_id);

        if (err != ESP_OK)
        {
            publish_command_response(
                command_id, "FAILED", "UART_TRANSACTION_ERROR");

            cJSON_Delete(root);
            return;
        }

        ESP_LOGI(TAG, "DISPENSE enviado a controlador | UART ID=%u", uart_id);

        /*
         * NO publicamos SUCCESS todavía.
         * Esperamos ACK / DONE / NACK.
         */
    }

    else if (strcmp(command, "STOP") == 0)
    {
        uint16_t uart_id;

        esp_err_t err =
            transaction_manager_create(command_id, "STOP", NULL, &uart_id);

        if (err != ESP_OK)
        {
            publish_command_response(
                command_id, "FAILED", "UART_TRANSACTION_ERROR");

            cJSON_Delete(root);
            return;
        }

        ESP_LOGI(TAG, "STOP enviado a controlador | UART ID=%u", uart_id);
    }

    //--------------------------------------------------
    // COMANDO DESCONOCIDO
    //--------------------------------------------------

    else
    {
        ESP_LOGW(TAG, "Comando no soportado: %s", command);

        publish_command_response(command_id, "REJECTED", "UNKNOWN_COMMAND");
    }

    cJSON_Delete(root);
}

//==================================================
// EVENT HANDLER MQTT
//==================================================

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        //--------------------------------------------------
        // CONECTADO
        //--------------------------------------------------

    case MQTT_EVENT_CONNECTED:
    {
        mqtt_connected = true;

        ESP_LOGI(TAG, "MQTT conectado");

        //--------------------------------------------------
        // SUSCRIBIR COMMAND REQUEST
        //--------------------------------------------------

        int msg_id =
            esp_mqtt_client_subscribe(mqtt_client, topic_command_request, 1);

        ESP_LOGI(TAG, "Suscrito a command/request | msg_id=%d", msg_id);

        //--------------------------------------------------
        // AVAILABILITY
        //--------------------------------------------------

        mqtt_manager_publish_online();

        //--------------------------------------------------
        // STATE
        //--------------------------------------------------

        mqtt_manager_publish_state();

        break;
    }

        //--------------------------------------------------
        // DESCONECTADO
        //--------------------------------------------------

    case MQTT_EVENT_DISCONNECTED:
    {
        mqtt_connected = false;

        ESP_LOGW(TAG, "MQTT desconectado");

        break;
    }

        //--------------------------------------------------
        // SUSCRIPCIÓN CONFIRMADA
        //--------------------------------------------------

    case MQTT_EVENT_SUBSCRIBED:
    {
        ESP_LOGI(TAG, "Suscripcion confirmada | msg_id=%d", event->msg_id);

        break;
    }

        //--------------------------------------------------
        // PUBLICACIÓN CONFIRMADA
        //--------------------------------------------------

    case MQTT_EVENT_PUBLISHED:
    {
        ESP_LOGD(TAG, "Publicacion confirmada | msg_id=%d", event->msg_id);

        break;
    }

        //--------------------------------------------------
        // MENSAJE RECIBIDO
        //--------------------------------------------------

    case MQTT_EVENT_DATA:
    {
        ESP_LOGI(TAG, "Mensaje MQTT recibido");

        //--------------------------------------------------
        // Verificar que corresponde a command/request
        //--------------------------------------------------

        if (event->topic_len == strlen(topic_command_request) &&

            strncmp(event->topic, topic_command_request, event->topic_len) == 0)
        {
            process_command(event->data, event->data_len);
        }

        break;
    }

        //--------------------------------------------------
        // ERROR
        //--------------------------------------------------

    case MQTT_EVENT_ERROR:
    {
        ESP_LOGE(TAG, "Error MQTT");

        break;
    }

    default:
        break;
    }
}

//==================================================
// INICIALIZAR MQTT
//==================================================

esp_err_t mqtt_manager_init(void)
{
    if (mqtt_client != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    //--------------------------------------------------
    // TOPICS
    //--------------------------------------------------

    build_topics();

    //--------------------------------------------------
    // URI
    //--------------------------------------------------

    static char broker_uri[128];

    snprintf(
        broker_uri,
        sizeof(broker_uri),
        "mqtt://%s:%d",
        MAIM_MQTT_HOST,
        MAIM_MQTT_PORT);

    //--------------------------------------------------
    // CLIENT ID
    //--------------------------------------------------

    static char client_id[64];

    snprintf(client_id, sizeof(client_id), "maim-%s", MAIM_DEVICE_ID);

    //--------------------------------------------------
    // LAST WILL
    //--------------------------------------------------

    static const char last_will[] = "{\"schema\":1,\"status\":\"offline\"}";

    //--------------------------------------------------
    // CONFIGURACIÓN
    //--------------------------------------------------

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,

        .credentials.client_id = client_id,

        .credentials.username = MAIM_MQTT_USER,

        .credentials.authentication.password = MAIM_MQTT_PASSWORD,

        .session.last_will.topic = topic_availability,

        .session.last_will.msg = last_will,

        .session.last_will.msg_len = 0,

        .session.last_will.qos = 1,

        .session.last_will.retain = true};

    //--------------------------------------------------
    // CREAR CLIENTE
    //--------------------------------------------------

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    if (mqtt_client == NULL)
    {
        ESP_LOGE(TAG, "No fue posible crear cliente MQTT");

        return ESP_FAIL;
    }

    //--------------------------------------------------
    // REGISTRAR EVENTOS
    //--------------------------------------------------

    esp_err_t err = esp_mqtt_client_register_event(
        mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG, "Error registrando eventos MQTT: %s", esp_err_to_name(err));

        return err;
    }

    //--------------------------------------------------
    // ARRANCAR CLIENTE
    //--------------------------------------------------

    err = esp_mqtt_client_start(mqtt_client);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error iniciando MQTT: %s", esp_err_to_name(err));

        return err;
    }

    ESP_LOGI(TAG, "Cliente MQTT iniciado");

    ESP_LOGI(TAG, "Broker: %s", broker_uri);

    return ESP_OK;
}

//==================================================
// ESTADO
//==================================================

bool mqtt_manager_is_connected(void)
{
    return mqtt_connected;
}

//==================================================
// AVAILABILITY ONLINE
//==================================================

esp_err_t mqtt_manager_publish_online(void)
{
    cJSON *root = cJSON_CreateObject();

    if (root == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddNumberToObject(root, "schema", 1);

    cJSON_AddNumberToObject(root, "ts", time_manager_get_timestamp());

    cJSON_AddStringToObject(root, "status", "online");

    esp_err_t err = publish_json(topic_availability, root, 1, true);

    cJSON_Delete(root);

    return err;
}

//==================================================
// AVAILABILITY SLEEPING
//==================================================

esp_err_t mqtt_manager_publish_sleeping(uint32_t next_wakeup_sec)
{
    cJSON *root = cJSON_CreateObject();

    if (root == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddNumberToObject(root, "schema", 1);

    cJSON_AddNumberToObject(root, "ts", time_manager_get_timestamp());

    cJSON_AddStringToObject(root, "status", "sleeping");

    cJSON_AddNumberToObject(root, "next_wakeup_sec", next_wakeup_sec);

    esp_err_t err = publish_json(topic_availability, root, 1, true);

    cJSON_Delete(root);

    return err;
}

//==================================================
// STATE / REPORTED
//==================================================

esp_err_t mqtt_manager_publish_state(void)
{
    if (!mqtt_connected)
    {
        return ESP_ERR_INVALID_STATE;
    }

    //--------------------------------------------------
    // DATOS WIFI
    //--------------------------------------------------

    char ip[16] = "0.0.0.0";

    wifi_manager_get_ip(ip, sizeof(ip));

    int8_t rssi = wifi_manager_get_rssi();

    //--------------------------------------------------
    // ROOT
    //--------------------------------------------------

    cJSON *root = cJSON_CreateObject();

    if (root == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddNumberToObject(root, "schema", 1);

    cJSON_AddNumberToObject(root, "ts", time_manager_get_timestamp());

    //--------------------------------------------------
    // DEVICE
    //--------------------------------------------------

    cJSON *device = cJSON_AddObjectToObject(root, "device");

    cJSON_AddStringToObject(device, "device_id", MAIM_DEVICE_ID);

    cJSON_AddStringToObject(device, "model", MAIM_MODEL);

    cJSON_AddStringToObject(device, "hw_rev", MAIM_HW_REV);

    //--------------------------------------------------
    // STATE
    //--------------------------------------------------

    cJSON *state = cJSON_AddObjectToObject(root, "state");

    const char *mode = device_manager_get_state("MODE");

    if (mode != NULL)
    {
        cJSON_AddStringToObject(state, "mode", mode);
    }

    const char *busy = device_manager_get_state("BUSY");

    if (busy != NULL)
    {
        cJSON_AddBoolToObject(state, "busy", strcmp(busy, "1") == 0);
    }

    const char *power_source = device_manager_get_state("POWER_SOURCE");

    if (power_source != NULL)
    {
        cJSON_AddStringToObject(state, "power_source", power_source);
    }

    const char *charging = device_manager_get_state("CHARGING");

    if (charging != NULL)
    {
        cJSON_AddBoolToObject(state, "charging", strcmp(charging, "1") == 0);
    }

    //--------------------------------------------------
    // METRICS
    //--------------------------------------------------

    cJSON *metrics = cJSON_AddObjectToObject(root, "metrics");

    int64_t water_total_ml;

    if (device_manager_get_metric_int64("WATER_TOTAL_ML", &water_total_ml) ==
        ESP_OK)
    {
        cJSON_AddNumberToObject(metrics, "water_total_ml", water_total_ml);
    }

    int64_t last_dispense_ml;

    if (device_manager_get_metric_int64(
            "LAST_DISPENSE_ML", &last_dispense_ml) == ESP_OK)
    {
        cJSON_AddNumberToObject(metrics, "last_dispense_ml", last_dispense_ml);
    }

    int64_t dispense_count;

    if (device_manager_get_metric_int64("DISPENSE_COUNT", &dispense_count) ==
        ESP_OK)
    {
        cJSON_AddNumberToObject(metrics, "dispense_count", dispense_count);
    }

    //--------------------------------------------------
    // SENSORS
    //--------------------------------------------------

    cJSON *sensors = cJSON_AddObjectToObject(root, "sensors");

    float battery_mv;

    if (device_manager_get_sensor_float("BATTERY_MV", &battery_mv) == ESP_OK)
    {
        cJSON_AddNumberToObject(sensors, "battery_mv", battery_mv);
    }

    float battery_pct;

    if (device_manager_get_sensor_float("BATTERY_PCT", &battery_pct) == ESP_OK)
    {
        cJSON_AddNumberToObject(sensors, "battery_pct", battery_pct);
    }

    //--------------------------------------------------
    // OUTPUTS
    //--------------------------------------------------

    cJSON *outputs = cJSON_AddObjectToObject(root, "outputs");

    bool valve;

    if (device_manager_get_output_bool("VALVE", &valve) == ESP_OK)
    {
        cJSON_AddBoolToObject(outputs, "valve", valve);
    }

    //--------------------------------------------------
    // CONFIG
    //--------------------------------------------------

    cJSON *config = cJSON_AddObjectToObject(root, "config");

    const char *dispense_time = device_manager_get_config("DISPENSE_TIME_MS");

    if (dispense_time != NULL)
    {
        char *endptr = NULL;

        long value = strtol(dispense_time, &endptr, 10);

        if (endptr != dispense_time && *endptr == '\0')
        {
            cJSON_AddNumberToObject(config, "dispense_time_ms", value);
        }
    }

    const char *pulses_per_liter =
        device_manager_get_config("PULSES_PER_LITER");

    if (pulses_per_liter != NULL)
    {
        char *endptr = NULL;

        float value = strtof(pulses_per_liter, &endptr);

        if (endptr != pulses_per_liter && *endptr == '\0')
        {
            cJSON_AddNumberToObject(config, "pulses_per_liter", value);
        }
    }

    //--------------------------------------------------
    // NETWORK
    //--------------------------------------------------

    cJSON *network = cJSON_AddObjectToObject(root, "network");

    cJSON_AddStringToObject(network, "type", "wifi");

    cJSON_AddBoolToObject(network, "connected", wifi_manager_is_connected());

    cJSON_AddNumberToObject(network, "rssi_dbm", rssi);

    cJSON_AddStringToObject(network, "ip", ip);

    //--------------------------------------------------
    // FIRMWARE
    //--------------------------------------------------

    cJSON *firmware = cJSON_AddObjectToObject(root, "firmware");

    //--------------------------------------------------
    // ESP32
    //--------------------------------------------------

    cJSON *esp32 = cJSON_AddObjectToObject(firmware, "esp32");

    cJSON_AddStringToObject(esp32, "version", MAIM_ESP_FW_VERSION);

    //--------------------------------------------------
    // CONTROLADOR ATMEGA
    //--------------------------------------------------

    const device_controller_info_t *controller_info =
        device_manager_get_controller_info();

    if (controller_info != NULL && controller_info->valid)
    {
        cJSON *controller = cJSON_AddObjectToObject(firmware, "controller");

        cJSON_AddStringToObject(controller, "mcu", controller_info->mcu);

        cJSON_AddStringToObject(controller, "model", controller_info->model);

        cJSON_AddStringToObject(controller, "hw_rev", controller_info->hw_rev);

        cJSON_AddStringToObject(
            controller, "version", controller_info->fw_version);
    }

    //--------------------------------------------------
    // ERRORS
    //--------------------------------------------------

    cJSON *errors = cJSON_AddArrayToObject(root, "errors");

    size_t error_count = device_manager_get_error_count();

    for (size_t i = 0; i < error_count; i++)
    {
        const device_error_entry_t *error = device_manager_get_error(i);

        if (error == NULL)
        {
            continue;
        }

        cJSON *error_json = cJSON_CreateObject();

        if (error_json == NULL)
        {
            continue;
        }

        cJSON_AddStringToObject(error_json, "code", error->code);

        cJSON_AddStringToObject(error_json, "severity", error->severity);

        cJSON_AddItemToArray(errors, error_json);
    }

    //--------------------------------------------------
    // PUBLICAR
    //--------------------------------------------------

    esp_err_t err = publish_json(topic_state_reported, root, 1, true);

    cJSON_Delete(root);

    return err;
}

//==================================================
// TELEMETRY
//==================================================

esp_err_t mqtt_manager_publish_telemetry(void)
{
    if (!mqtt_connected)
    {
        return ESP_ERR_INVALID_STATE;
    }

    cJSON *root = cJSON_CreateObject();

    if (root == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddNumberToObject(root, "schema", 1);

    cJSON_AddNumberToObject(root, "ts", time_manager_get_timestamp());

    //--------------------------------------------------
    // NETWORK
    //--------------------------------------------------

    cJSON *network = cJSON_AddObjectToObject(root, "network");

    cJSON_AddNumberToObject(network, "rssi_dbm", wifi_manager_get_rssi());

    //--------------------------------------------------
    // PUBLICAR
    //--------------------------------------------------

    esp_err_t err = publish_json(topic_telemetry, root, 0, false);

    cJSON_Delete(root);

    return err;
}

//==================================================
// EVENT
//==================================================

esp_err_t mqtt_manager_publish_event(
    const char *event_type, const uart_protocol_frame_t *frame)
{
    if (event_type == NULL || frame == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!mqtt_connected)
    {
        ESP_LOGW(TAG, "EVENT no publicado: MQTT desconectado");

        return ESP_ERR_INVALID_STATE;
    }

    //--------------------------------------------------
    // ROOT
    //--------------------------------------------------

    cJSON *root = cJSON_CreateObject();

    if (root == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    //--------------------------------------------------
    // EVENT ID
    //--------------------------------------------------

    char event_id[64];

    generate_event_id(event_id, sizeof(event_id));

    cJSON_AddNumberToObject(root, "schema", 1);

    cJSON_AddStringToObject(root, "event_id", event_id);

    cJSON_AddNumberToObject(root, "ts", time_manager_get_timestamp());

    cJSON_AddStringToObject(root, "type", event_type);

    //--------------------------------------------------
    // SEVERITY
    //
    // Los eventos normales son INFO.
    // Los errores tendrán su propio manejo después.
    //--------------------------------------------------

    cJSON_AddStringToObject(root, "severity", "info");

    //--------------------------------------------------
    // DATA
    //--------------------------------------------------

    cJSON *data = cJSON_AddObjectToObject(root, "data");

    //--------------------------------------------------
    // DISPENSE_STARTED
    //--------------------------------------------------

    if (strcmp(event_type, "DISPENSE_STARTED") == 0)
    {
        /*
         * Sin parámetros obligatorios por ahora.
         */
    }

    //--------------------------------------------------
    // DISPENSE_COMPLETED
    //
    // <EVENT,DISPENSE_COMPLETED,VOLUME_ML,TOTAL_ML>
    //--------------------------------------------------

    else if (strcmp(event_type, "DISPENSE_COMPLETED") == 0)
    {
        if (frame->field_count != 3)
        {
            ESP_LOGW(
                TAG, "DISPENSE_COMPLETED requiere volume_ml y water_total_ml");

            cJSON_Delete(root);

            return ESP_ERR_INVALID_ARG;
        }

        char *endptr = NULL;

        long volume_ml = strtol(frame->fields[1], &endptr, 10);

        if (endptr == frame->fields[1] || *endptr != '\0')
        {
            cJSON_Delete(root);

            return ESP_ERR_INVALID_ARG;
        }

        long total_ml = strtol(frame->fields[2], &endptr, 10);

        if (endptr == frame->fields[2] || *endptr != '\0')
        {
            cJSON_Delete(root);

            return ESP_ERR_INVALID_ARG;
        }

        cJSON_AddNumberToObject(data, "volume_ml", volume_ml);

        cJSON_AddNumberToObject(data, "water_total_ml", total_ml);
    }

    //--------------------------------------------------
    // DISPENSE_CANCELLED
    //
    // <EVENT,DISPENSE_CANCELLED,VOLUME_ML>
    //--------------------------------------------------

    else if (strcmp(event_type, "DISPENSE_CANCELLED") == 0)
    {
        if (frame->field_count >= 2)
        {
            char *endptr = NULL;

            long volume_ml = strtol(frame->fields[1], &endptr, 10);

            if (endptr != frame->fields[1] && *endptr == '\0')
            {
                cJSON_AddNumberToObject(data, "volume_ml", volume_ml);
            }
        }
    }

    //--------------------------------------------------
    // CALIBRATION_COMPLETED
    //
    // <EVENT,CALIBRATION_COMPLETED,PULSES_PER_LITER>
    //--------------------------------------------------

    else if (strcmp(event_type, "CALIBRATION_COMPLETED") == 0)
    {
        if (frame->field_count != 2)
        {
            ESP_LOGW(TAG, "CALIBRATION_COMPLETED requiere pulses_per_liter");

            cJSON_Delete(root);

            return ESP_ERR_INVALID_ARG;
        }

        char *endptr = NULL;

        double pulses_per_liter = strtod(frame->fields[1], &endptr);

        if (endptr == frame->fields[1] || *endptr != '\0')
        {
            cJSON_Delete(root);

            return ESP_ERR_INVALID_ARG;
        }

        cJSON_AddNumberToObject(data, "pulses_per_liter", pulses_per_liter);
    }

    //--------------------------------------------------
    // PROGRAMMING_CHANGED
    //
    // <EVENT,PROGRAMMING_CHANGED,DISPENSE_TIME_MS>
    //--------------------------------------------------

    else if (strcmp(event_type, "PROGRAMMING_CHANGED") == 0)
    {
        if (frame->field_count != 2)
        {
            ESP_LOGW(TAG, "PROGRAMMING_CHANGED requiere dispense_time_ms");

            cJSON_Delete(root);

            return ESP_ERR_INVALID_ARG;
        }

        char *endptr = NULL;

        long dispense_time_ms = strtol(frame->fields[1], &endptr, 10);

        if (endptr == frame->fields[1] || *endptr != '\0')
        {
            cJSON_Delete(root);

            return ESP_ERR_INVALID_ARG;
        }

        cJSON_AddNumberToObject(data, "dispense_time_ms", dispense_time_ms);
    }

    //--------------------------------------------------
    // EVENTO GENERICO
    //
    // Conservamos los parámetros como strings.
    //--------------------------------------------------

    else
    {
        for (uint8_t i = 1; i < frame->field_count; i++)
        {
            char key[16];

            snprintf(key, sizeof(key), "param_%u", i);

            cJSON_AddStringToObject(data, key, frame->fields[i]);
        }
    }

    //--------------------------------------------------
    // PUBLICAR
    //--------------------------------------------------

    esp_err_t err = publish_json(topic_event, root, 1, false);

    cJSON_Delete(root);

    return err;
}

esp_err_t mqtt_manager_publish_transaction_response(
    const char *command_id, const char *status, const char *reason)
{
    if (command_id == NULL || status == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_CreateObject();

    if (root == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddNumberToObject(root, "schema", 1);

    cJSON_AddStringToObject(root, "command_id", command_id);

    cJSON_AddNumberToObject(root, "ts", time_manager_get_timestamp());

    cJSON_AddStringToObject(root, "status", status);

    if (reason != NULL)
    {
        cJSON *error = cJSON_AddObjectToObject(root, "error");

        cJSON_AddStringToObject(error, "code", reason);
    }

    esp_err_t err = publish_json(topic_command_response, root, 1, false);

    cJSON_Delete(root);

    return err;
}