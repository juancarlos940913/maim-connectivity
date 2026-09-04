#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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



//==================================================
// LOG
//==================================================

static const char *TAG = "MAIM";

//==================================================
// INICIALIZAR NVS
//==================================================

static void inicializar_nvs(void)
{
    esp_err_t err =
        nvs_flash_init();

    if (
        err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND
    )
    {
        ESP_LOGW(
            TAG,
            "NVS requiere reinicializacion"
        );

        ESP_ERROR_CHECK(
            nvs_flash_erase()
        );

        err =
            nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);

    ESP_LOGI(
        TAG,
        "NVS inicializada correctamente"
    );
}

//==================================================
// CALLBACK DEL PROTOCOLO UART
//==================================================

static void uart_frame_received(
    const uart_protocol_frame_t *frame
)
{
    esp_err_t err =
        device_manager_process_frame(
            frame
        );

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "Device Manager rechazo trama %s: %s",
            uart_protocol_type_to_string(
                frame->type
            ),
            esp_err_to_name(err)
        );

        return;
    }

    //--------------------------------------------------
    // SOLO DURANTE PRUEBAS
    //--------------------------------------------------

    if (
        frame->type == UART_FRAME_STATE ||
        frame->type == UART_FRAME_METRIC ||
        frame->type == UART_FRAME_SENSOR ||
        frame->type == UART_FRAME_OUTPUT ||
        frame->type == UART_FRAME_ERROR ||
        frame->type == UART_FRAME_ERROR_CLEAR
    )
    {
        device_manager_print_status();
    }
}


static void device_transaction_received(
    uart_frame_type_t type,
    uint16_t transaction_id,
    const uart_protocol_frame_t *frame
)
{
    esp_err_t err =
        transaction_manager_process_uart_response(
            type,
            transaction_id,
            frame
        );

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "Respuesta UART no procesada: %s",
            esp_err_to_name(err)
        );
    }
}

//==================================================
// TAREA DE ENTRADA SERIAL DE PRUEBA
//==================================================

static void serial_test_task(
    void *arg
)
{
    ESP_LOGI(
        TAG,
        "Terminal UART de prueba lista"
    );

    ESP_LOGI(
        TAG,
        "Escribe tramas como:"
    );

    ESP_LOGI(
        TAG,
        "<STATE,MODE,STANDBY>"
    );

    while (1)
    {
        int c = getchar();

        if (c >= 0)
        {
            uart_protocol_process_char(
                (char)c
            );
        }
        else
        {
            /*
             * stdin UART es no bloqueante por defecto.
             * Evitamos consumir CPU continuamente.
             */
            vTaskDelay(
                pdMS_TO_TICKS(10)
            );
        }
    }
}

//==================================================
// CALLBACK EVENTOS DEL CONTROLADOR
//==================================================

static void device_event_received(
    const char *event_type,
    const uart_protocol_frame_t *frame
)
{
    ESP_LOGI(
        TAG,
        "Evento del controlador recibido: %s",
        event_type
    );

    esp_err_t err =
        mqtt_manager_publish_event(
            event_type,
            frame
        );

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "No fue posible publicar EVENT MQTT: %s",
            esp_err_to_name(err)
        );
    }
}

static void transaction_result_received(
    const char *mqtt_command_id,
    transaction_state_t state,
    const char *reason
)
{
    switch (state)
    {
        //--------------------------------------------------
        // ACK
        //--------------------------------------------------

        case TRANSACTION_STATE_ACKED:
        {
            mqtt_manager_publish_transaction_response(
                mqtt_command_id,
                "RECEIVED",
                NULL
            );

            break;
        }

        //--------------------------------------------------
        // DONE
        //--------------------------------------------------

        case TRANSACTION_STATE_COMPLETED:
        {
            mqtt_manager_publish_transaction_response(
                mqtt_command_id,
                "SUCCESS",
                NULL
            );

            break;
        }

        //--------------------------------------------------
        // NACK
        //--------------------------------------------------

        case TRANSACTION_STATE_REJECTED:
        {
            mqtt_manager_publish_transaction_response(
                mqtt_command_id,
                "REJECTED",
                reason
            );

            break;
        }

        //--------------------------------------------------
        // TIMEOUT / FALLO
        //--------------------------------------------------

        case TRANSACTION_STATE_FAILED:
        {
            mqtt_manager_publish_transaction_response(
                mqtt_command_id,
                "FAILED",
                reason
            );

            break;
        }

        default:
            break;
    }
}

//==================================================
// TAREA DE SUPERVISION DE TRANSACCIONES
//==================================================

static void transaction_timeout_task(
    void *arg
)
{
    ESP_LOGI(
        TAG,
        "Supervisor de timeouts iniciado"
    );

    while (1)
    {
        //--------------------------------------------------
        // Revisar transacciones activas
        //--------------------------------------------------

        transaction_manager_process_timeouts();

        //--------------------------------------------------
        // Revisar cada 250 ms
        //--------------------------------------------------

        vTaskDelay(
            pdMS_TO_TICKS(250)
        );
    }
}

//==================================================
// APP MAIN
//==================================================

void app_main(void)
{
    ESP_LOGI(
        TAG,
        "================================"
    );

    ESP_LOGI(
        TAG,
        "      MAIM CONNECTIVITY"
    );

    ESP_LOGI(
        TAG,
        "================================"
    );

    ESP_LOGI(
        TAG,
        "Device ID : %s",
        MAIM_DEVICE_ID
    );

    ESP_LOGI(
        TAG,
        "Model     : %s",
        MAIM_MODEL
    );

    ESP_LOGI(
        TAG,
        "HW Rev    : %s",
        MAIM_HW_REV
    );

    ESP_LOGI(
        TAG,
        "FW ESP32  : %s",
        MAIM_ESP_FW_VERSION
    );

    //--------------------------------------------------
    // NVS
    //--------------------------------------------------

    inicializar_nvs();

  //--------------------------------------------------
    // UART PROTOCOL
    //--------------------------------------------------

    uart_protocol_init();
    device_manager_init();
    transaction_manager_init();

    transaction_manager_set_result_callback(
    transaction_result_received
    );

    BaseType_t timeout_task_result =
    xTaskCreate(
        transaction_timeout_task,
        "transaction_timeout",
        3072,
        NULL,
        5,
        NULL
    );

if (
    timeout_task_result ==
    pdPASS
)
{
    ESP_LOGI(
        TAG,
        "Supervisor de timeouts creado"
    );
}
else
{
    ESP_LOGE(
        TAG,
        "ERROR creando supervisor de timeouts"
    );
}

    uart_protocol_set_callback(
    uart_frame_received
    );

    device_manager_set_event_callback(
    device_event_received
    );

    device_manager_set_transaction_callback(
    device_transaction_received
    );



BaseType_t task_result = xTaskCreate(
    serial_test_task,
    "serial_test",
    4096,
    NULL,
    5,
    NULL
);

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

    ESP_ERROR_CHECK(
        wifi_manager_init(
            MAIM_WIFI_SSID,
            MAIM_WIFI_PASSWORD
        )
    );

    //--------------------------------------------------
    // ESPERAR WIFI
    //--------------------------------------------------

    ESP_LOGI(
        TAG,
        "Esperando WiFi..."
    );

    while (!wifi_manager_is_connected())
    {
        vTaskDelay(
            pdMS_TO_TICKS(500)
        );
    }

    ESP_LOGI(
        TAG,
        "WiFi disponible"
    );
    
    //--------------------------------------------------
// SINCRONIZACION DE HORA
//--------------------------------------------------

esp_err_t time_err =
    time_manager_init();

if (time_err != ESP_OK)
{
    ESP_LOGW(
        TAG,
        "Continuando sin hora sincronizada"
    );
}

    //--------------------------------------------------
    // MQTT
    //--------------------------------------------------

    ESP_ERROR_CHECK(
        mqtt_manager_init()
    );

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

            if (
                wifi_manager_get_ip(
                    ip,
                    sizeof(ip)
                ) == ESP_OK
            )
            {
                ESP_LOGI(
                    TAG,
                    "WiFi OK | IP: %s | RSSI: %d dBm | MQTT: %s",
                    ip,
                    wifi_manager_get_rssi(),
                    mqtt_manager_is_connected()
                        ? "ONLINE"
                        : "OFFLINE"
                );
            }
        }

        //--------------------------------------------------
        // TELEMETRÍA CADA 30 SEGUNDOS
        //--------------------------------------------------

        telemetry_counter += 5;

        if (
            telemetry_counter >= 30 &&
            mqtt_manager_is_connected()
        )
        {
            telemetry_counter = 0;

            mqtt_manager_publish_telemetry();
        }

        vTaskDelay(
            pdMS_TO_TICKS(5000)
        );
    }
}