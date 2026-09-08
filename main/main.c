#include <stdio.h>

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_err.h"

#include "nvs_flash.h"

#include "maim_config.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "time_manager.h"
#include "uart_protocol.h"
#include "device_manager.h"
#include "transaction_manager.h"
#include "uart_transport.h"

//==================================================
// ESTADO DE SINCRONIZACION
//==================================================

static volatile bool controller_state_synced = false;
static volatile bool controller_info_synced = false;

static EventGroupHandle_t controller_sync_event_group = NULL;

#define CONTROLLER_SYNC_STATE_BIT BIT0
#define CONTROLLER_SYNC_INFO_BIT BIT1
#define CONTROLLER_SYNC_STATE_DONE_BIT BIT2
#define CONTROLLER_SYNC_INFO_DONE_BIT BIT3

#define CONTROLLER_SYNC_SUCCESS_BITS                                           \
    (CONTROLLER_SYNC_STATE_BIT | CONTROLLER_SYNC_INFO_BIT)

#define CONTROLLER_SYNC_DONE_BITS                                              \
    (CONTROLLER_SYNC_STATE_DONE_BIT | CONTROLLER_SYNC_INFO_DONE_BIT)

#define CONTROLLER_SYNC_TIMEOUT_MS 5000

//==================================================
// LOG
//==================================================

static const char *TAG = "MAIM";

//==================================================
// INICIALIZAR NVS
//==================================================

static void inicializar_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS requiere reinicializacion");

        ESP_ERROR_CHECK(nvs_flash_erase());

        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "NVS inicializada correctamente");
}

//==================================================
// CALLBACK DEL PROTOCOLO UART
//==================================================

static void uart_frame_received(const uart_protocol_frame_t *frame)
{
    esp_err_t err = device_manager_process_frame(frame);

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "Device Manager rechazo trama %s: %s",
            uart_protocol_type_to_string(frame->type),
            esp_err_to_name(err));

        return;
    }
}

static void device_transaction_received(
    uart_frame_type_t type,
    uint16_t transaction_id,
    const uart_protocol_frame_t *frame)
{
    esp_err_t err =
        transaction_manager_process_uart_response(type, transaction_id, frame);

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Respuesta UART no procesada: %s", esp_err_to_name(err));
    }
}

//==================================================
// TAREA DE ENTRADA SERIAL DE PRUEBA
//==================================================

static void serial_test_task(void *arg)
{
    ESP_LOGI(TAG, "Terminal UART de prueba lista");

    ESP_LOGI(TAG, "Escribe tramas como:");

    ESP_LOGI(TAG, "<STATE,MODE,STANDBY>");

    while (1)
    {
        int c = getchar();

        if (c >= 0)
        {
            uart_protocol_process_char((char)c);
        }
        else
        {
            /*
             * stdin UART es no bloqueante por defecto.
             * Evitamos consumir CPU continuamente.
             */
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

//==================================================
// CALLBACK EVENTOS DEL CONTROLADOR
//==================================================

static void device_event_received(
    const char *event_type, const uart_protocol_frame_t *frame)
{
    ESP_LOGI(TAG, "Evento del controlador recibido: %s", event_type);

    esp_err_t err = mqtt_manager_publish_event(event_type, frame);

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "No fue posible publicar EVENT MQTT: %s",
            esp_err_to_name(err));
    }
}

static void transaction_result_received(
    transaction_origin_t origin,
    const char *mqtt_command_id,
    const char *command,
    transaction_state_t state,
    const char *reason)
{
    //--------------------------------------------------
    // TRANSACCIONES INTERNAS
    //--------------------------------------------------

    if (origin == TRANSACTION_ORIGIN_INTERNAL)
    {
        if (state == TRANSACTION_STATE_ACKED)
        {
            ESP_LOGI(TAG, "Transaccion interna ACK | CMD=%s", command);

            return;
        }

        if (state == TRANSACTION_STATE_COMPLETED)
        {
            //--------------------------------------------------
            // GET_STATE COMPLETADO
            //--------------------------------------------------

            if (strcmp(command, "GET_STATE") == 0)
            {
                controller_state_synced = true;

                if (controller_sync_event_group != NULL)
                {
                    xEventGroupSetBits(
                        controller_sync_event_group,
                        CONTROLLER_SYNC_STATE_BIT |
                            CONTROLLER_SYNC_STATE_DONE_BIT);
                }

                ESP_LOGI(TAG, "Estado del controlador sincronizado");
            }

            //--------------------------------------------------
            // GET_INFO COMPLETADO
            //--------------------------------------------------

            else if (strcmp(command, "GET_INFO") == 0)
            {
                const device_controller_info_t *controller_info =
                    device_manager_get_controller_info();

                if (controller_info != NULL && controller_info->valid)
                {
                    controller_info_synced = true;
                    if (controller_sync_event_group != NULL)
                    {
                        xEventGroupSetBits(
                            controller_sync_event_group,
                            CONTROLLER_SYNC_INFO_BIT |
                                CONTROLLER_SYNC_INFO_DONE_BIT);
                    }

                    ESP_LOGI(TAG, "Identidad del controlador sincronizada");
                }
                else
                {
                    ESP_LOGW(TAG, "GET_INFO completo sin identidad valida");
                }
            }

            ESP_LOGI(TAG, "Transaccion interna completada | CMD=%s", command);

            return;
        }

        if (state == TRANSACTION_STATE_REJECTED ||
            state == TRANSACTION_STATE_FAILED)
        {
            //--------------------------------------------------
            // MARCAR SINCRONIZACION COMO FINALIZADA
            //--------------------------------------------------

            if (controller_sync_event_group != NULL)
            {
                if (strcmp(command, "GET_STATE") == 0)
                {
                    xEventGroupSetBits(
                        controller_sync_event_group,
                        CONTROLLER_SYNC_STATE_DONE_BIT);
                }
                else if (strcmp(command, "GET_INFO") == 0)
                {
                    xEventGroupSetBits(
                        controller_sync_event_group,
                        CONTROLLER_SYNC_INFO_DONE_BIT);
                }
            }

            ESP_LOGW(
                TAG,
                "Transaccion interna fallo | CMD=%s | Reason=%s",
                command,
                reason != NULL ? reason : "UNKNOWN");

            return;
        }

        return;
    }

    switch (state)
    {
        //--------------------------------------------------
        // ACK
        //--------------------------------------------------

    case TRANSACTION_STATE_ACKED:
    {
        mqtt_manager_publish_transaction_response(
            mqtt_command_id, "RECEIVED", NULL);

        break;
    }

        //--------------------------------------------------
        // DONE
        //--------------------------------------------------

    case TRANSACTION_STATE_COMPLETED:
    {
        //--------------------------------------------------
        // GET_STATE
        //
        // El DONE llega despues de SNAPSHOT END, por lo
        // tanto el Device Manager ya contiene el estado
        // actualizado del controlador.
        //--------------------------------------------------

        if (strcmp(command, "GET_STATE") == 0)
        {
            esp_err_t err = mqtt_manager_publish_state();

            if (err != ESP_OK)
            {
                ESP_LOGW(
                    TAG,
                    "No fue posible publicar state/reported: %s",
                    esp_err_to_name(err));
            }
        }

        //--------------------------------------------------
        // RESULTADO DE LA TRANSACCION
        //--------------------------------------------------

        mqtt_manager_publish_transaction_response(
            mqtt_command_id, "SUCCESS", NULL);

        break;
    }

        //--------------------------------------------------
        // NACK
        //--------------------------------------------------

    case TRANSACTION_STATE_REJECTED:
    {
        mqtt_manager_publish_transaction_response(
            mqtt_command_id, "REJECTED", reason);

        break;
    }

        //--------------------------------------------------
        // TIMEOUT / FALLO
        //--------------------------------------------------

    case TRANSACTION_STATE_FAILED:
    {
        mqtt_manager_publish_transaction_response(
            mqtt_command_id, "FAILED", reason);

        break;
    }

    default:
        break;
    }
}

//==================================================
// TAREA DE SUPERVISION DE TRANSACCIONES
//==================================================

static void transaction_timeout_task(void *arg)
{
    ESP_LOGI(TAG, "Supervisor de timeouts iniciado");

    while (1)
    {
        //--------------------------------------------------
        // Revisar transacciones activas
        //--------------------------------------------------

        transaction_manager_process_timeouts();

        //--------------------------------------------------
        // Revisar cada 250 ms
        //--------------------------------------------------

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

//==================================================
// SINCRONIZACION INICIAL CON CONTROLADOR
//==================================================

static esp_err_t sync_controller_state(void)
{
    controller_state_synced = false;
    if (controller_sync_event_group != NULL)
    {
        xEventGroupClearBits(
            controller_sync_event_group,
            CONTROLLER_SYNC_STATE_BIT | CONTROLLER_SYNC_STATE_DONE_BIT);
    }
    uint16_t uart_id = 0;

    esp_err_t err =
        transaction_manager_create_internal("GET_STATE", NULL, &uart_id);

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "No fue posible iniciar sincronizacion con controlador: %s",
            esp_err_to_name(err));

        return err;
    }

    ESP_LOGI(
        TAG, "Sincronizacion con controlador iniciada | UART ID=%u", uart_id);

    return ESP_OK;
}

//==================================================
// SINCRONIZACION DE IDENTIDAD DEL CONTROLADOR
//==================================================

static esp_err_t sync_controller_info(void)
{
    controller_state_synced = false;
    if (controller_sync_event_group != NULL)
    {
        xEventGroupClearBits(
            controller_sync_event_group,
            CONTROLLER_SYNC_INFO_BIT | CONTROLLER_SYNC_INFO_DONE_BIT);
    }
    uint16_t uart_id = 0;

    esp_err_t err =
        transaction_manager_create_internal("GET_INFO", NULL, &uart_id);

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "No fue posible iniciar sincronizacion de identidad: %s",
            esp_err_to_name(err));

        return err;
    }

    ESP_LOGI(TAG, "Sincronizacion de identidad iniciada | UART ID=%u", uart_id);

    return ESP_OK;
}

//==================================================
// ESPERAR SINCRONIZACION DEL CONTROLADOR
//==================================================

static bool wait_for_controller_sync(void)
{
    if (controller_sync_event_group == NULL)
    {
        ESP_LOGE(TAG, "EventGroup de sincronizacion no disponible");

        return false;
    }

    ESP_LOGI(TAG, "Esperando sincronizacion completa del controlador...");

    EventBits_t bits = xEventGroupWaitBits(
        controller_sync_event_group,
        CONTROLLER_SYNC_DONE_BITS,
        pdFALSE,
        pdTRUE,
        pdMS_TO_TICKS(CONTROLLER_SYNC_TIMEOUT_MS));

    bool state_ready = (bits & CONTROLLER_SYNC_STATE_BIT) != 0;

    bool info_ready = (bits & CONTROLLER_SYNC_INFO_BIT) != 0;

    bool state_done = (bits & CONTROLLER_SYNC_STATE_DONE_BIT) != 0;

    bool info_done = (bits & CONTROLLER_SYNC_INFO_DONE_BIT) != 0;

    if (state_ready && info_ready)
    {
        ESP_LOGI(TAG, "Controlador completamente sincronizado");

        return true;
    }

    ESP_LOGW(
        TAG,
        "Sincronizacion incompleta | STATE=%s | INFO=%s | "
        "STATE_DONE=%s | INFO_DONE=%s",
        state_ready ? "OK" : "FAILED",
        info_ready ? "OK" : "FAILED",
        state_done ? "YES" : "NO",
        info_done ? "YES" : "NO");

    return false;
}

//==================================================
// APP MAIN
//==================================================

void app_main(void)
{
    ESP_LOGI(TAG, "================================");

    ESP_LOGI(TAG, "      MAIM CONNECTIVITY");

    ESP_LOGI(TAG, "================================");

    ESP_LOGI(TAG, "Device ID : %s", MAIM_DEVICE_ID);

    ESP_LOGI(TAG, "Model     : %s", MAIM_MODEL);

    ESP_LOGI(TAG, "HW Rev    : %s", MAIM_HW_REV);

    ESP_LOGI(TAG, "FW ESP32  : %s", MAIM_ESP_FW_VERSION);

    //--------------------------------------------------
    // NVS
    //--------------------------------------------------

    inicializar_nvs();

    //--------------------------------------------------
    // UART TRANSPORT
    //--------------------------------------------------

    ESP_ERROR_CHECK(uart_transport_init());

    //--------------------------------------------------
    // UART PROTOCOL
    //--------------------------------------------------

    uart_protocol_init();
    device_manager_init();
    transaction_manager_init();

    //--------------------------------------------------
    // EVENT GROUP DE SINCRONIZACION
    //--------------------------------------------------

    controller_sync_event_group = xEventGroupCreate();

    if (controller_sync_event_group == NULL)
    {
        ESP_LOGE(TAG, "ERROR creando EventGroup de sincronizacion");

        abort();
    }

    ESP_LOGI(TAG, "EventGroup de sincronizacion creado");

    //--------------------------------------------------
    // CALLBACKS UART / DEVICE / TRANSACTIONS
    //
    // Todos deben quedar registrados ANTES de arrancar
    // la tarea RX para evitar perder tramas tempranas.
    //--------------------------------------------------

    uart_protocol_set_callback(uart_frame_received);

    device_manager_set_event_callback(device_event_received);

    device_manager_set_transaction_callback(device_transaction_received);

    transaction_manager_set_result_callback(transaction_result_received);

    //--------------------------------------------------
    // UART RX
    //--------------------------------------------------

    ESP_ERROR_CHECK(uart_transport_start_rx());

    //--------------------------------------------------
    // SUPERVISOR DE TRANSACCIONES
    //--------------------------------------------------

    BaseType_t timeout_task_result = xTaskCreate(
        transaction_timeout_task, "transaction_timeout", 3072, NULL, 5, NULL);

    if (timeout_task_result == pdPASS)
    {
        ESP_LOGI(TAG, "Supervisor de timeouts creado");
    }
    else
    {
        ESP_LOGE(TAG, "ERROR creando supervisor de timeouts");
    }

    //--------------------------------------------------
    // SINCRONIZACION INICIAL DEL CONTROLADOR
    //--------------------------------------------------

    esp_err_t sync_err = sync_controller_state();

    if (sync_err != ESP_OK)
    {
        ESP_LOGW(TAG, "Continuando arranque sin sincronizacion inicial");
    }

    //--------------------------------------------------
    // SINCRONIZACION DE IDENTIDAD DEL CONTROLADOR
    //--------------------------------------------------

    esp_err_t info_sync_err = sync_controller_info();

    if (info_sync_err != ESP_OK)
    {
        ESP_LOGW(TAG, "Continuando arranque sin identidad del controlador");
    }

    BaseType_t task_result =
        xTaskCreate(serial_test_task, "serial_test", 4096, NULL, 5, NULL);

    if (task_result == pdPASS)
    {
        ESP_LOGI(TAG, "Tarea serial_test creada correctamente");
    }
    else
    {
        ESP_LOGE(TAG, "ERROR creando tarea serial_test");
    }

    //--------------------------------------------------
    // WIFI
    //--------------------------------------------------

    ESP_ERROR_CHECK(wifi_manager_init(MAIM_WIFI_SSID, MAIM_WIFI_PASSWORD));

    //--------------------------------------------------
    // ESPERAR WIFI
    //--------------------------------------------------

    ESP_LOGI(TAG, "Esperando WiFi...");

    while (!wifi_manager_is_connected())
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "WiFi disponible");

    //--------------------------------------------------
    // SINCRONIZACION DE HORA
    //--------------------------------------------------

    esp_err_t time_err = time_manager_init();

    if (time_err != ESP_OK)
    {
        ESP_LOGW(
            TAG, "Hora no sincronizada | MQTT continuara en modo degradado");
    }
    else
    {
        ESP_LOGI(TAG, "Hora del sistema lista para MQTT");
    }

    //--------------------------------------------------
    // ESPERAR CONTROLADOR
    //--------------------------------------------------

    bool controller_ready = wait_for_controller_sync();

    if (!controller_ready)
    {
        ESP_LOGW(
            TAG, "Continuando MQTT con controlador parcialmente sincronizado");
    }

    //--------------------------------------------------
    // MQTT
    //--------------------------------------------------

    ESP_ERROR_CHECK(mqtt_manager_init());

    //--------------------------------------------------
    // LOOP PRINCIPAL DE PRUEBA
    //--------------------------------------------------

    uint32_t telemetry_counter = 0;

    while (1)
    {
        //--------------------------------------------------
        // LOG WIFI
        //--------------------------------------------------

        if (wifi_manager_is_connected())
        {
            char ip[16];

            if (wifi_manager_get_ip(ip, sizeof(ip)) == ESP_OK)
            {
                ESP_LOGI(
                    TAG,
                    "WiFi OK | IP: %s | RSSI: %d dBm | MQTT: %s",
                    ip,
                    wifi_manager_get_rssi(),
                    mqtt_manager_is_connected() ? "ONLINE" : "OFFLINE");
            }
        }

        //--------------------------------------------------
        // TELEMETRÍA CADA 30 SEGUNDOS
        //--------------------------------------------------

        telemetry_counter += 5;

        if (telemetry_counter >= 30 && mqtt_manager_is_connected())
        {
            telemetry_counter = 0;

            mqtt_manager_publish_telemetry();
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}