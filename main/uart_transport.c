#include "uart_transport.h"

#include <stdbool.h>

#include "driver/uart.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "uart_protocol.h"

//==================================================
// LOG
//==================================================

static const char *TAG = "UART_TRANSPORT";

//==================================================
// CONFIGURACION
//==================================================

#define MAIM_UART_PORT UART_NUM_2

#define MAIM_UART_TX_PIN 17
#define MAIM_UART_RX_PIN 16

#define MAIM_UART_BAUD_RATE 9600

#define MAIM_UART_RX_BUFFER_SIZE 512

//==================================================
// ESTADO
//==================================================

static bool initialized = false;
static bool rx_started = false;

//==================================================
// INICIALIZACION
//==================================================

esp_err_t uart_transport_init(void)
{
    if (initialized)
    {
        return ESP_OK;
    }

    //--------------------------------------------------
    // CONFIGURACION DEL UART
    //--------------------------------------------------

    const uart_config_t uart_config = {
        .baud_rate = MAIM_UART_BAUD_RATE,

        .data_bits = UART_DATA_8_BITS,

        .parity = UART_PARITY_DISABLE,

        .stop_bits = UART_STOP_BITS_1,

        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,

        .source_clk = UART_SCLK_DEFAULT};

    //--------------------------------------------------
    // APLICAR CONFIGURACION
    //--------------------------------------------------

    esp_err_t err = uart_param_config(MAIM_UART_PORT, &uart_config);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error configurando UART: %s", esp_err_to_name(err));

        return err;
    }

    //--------------------------------------------------
    // ASIGNAR PINES
    //--------------------------------------------------

    err = uart_set_pin(
        MAIM_UART_PORT,
        MAIM_UART_TX_PIN,
        MAIM_UART_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG, "Error configurando pines UART: %s", esp_err_to_name(err));

        return err;
    }

    //--------------------------------------------------
    // INSTALAR DRIVER
    //
    // RX buffer habilitado.
    // TX buffer = 0 por ahora.
    // Sin queue de eventos en este bloque.
    //--------------------------------------------------

    err = uart_driver_install(
        MAIM_UART_PORT, MAIM_UART_RX_BUFFER_SIZE, 0, 0, NULL, 0);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error instalando driver UART: %s", esp_err_to_name(err));

        return err;
    }

    //--------------------------------------------------
    // LISTO
    //--------------------------------------------------

    initialized = true;

    ESP_LOGI(
        TAG,
        "UART interno inicializado | UART%d | TX=%d | RX=%d | %d baud",
        MAIM_UART_PORT,
        MAIM_UART_TX_PIN,
        MAIM_UART_RX_PIN,
        MAIM_UART_BAUD_RATE);

    return ESP_OK;
}

//==================================================
// TRANSMISION
//==================================================

esp_err_t uart_transport_send(const char *data, size_t length)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "Intento de TX con UART no inicializado");

        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL || length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int bytes_written = uart_write_bytes(MAIM_UART_PORT, data, length);

    if (bytes_written < 0)
    {
        ESP_LOGE(TAG, "Error escribiendo UART");

        return ESP_FAIL;
    }

    if ((size_t)bytes_written != length)
    {
        ESP_LOGE(
            TAG,
            "TX UART incompleto: %d/%u bytes",
            bytes_written,
            (unsigned int)length);

        return ESP_FAIL;
    }

    esp_err_t wait_err = uart_wait_tx_done(MAIM_UART_PORT, pdMS_TO_TICKS(100));

    if (wait_err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Timeout esperando fin de TX UART: %s",
            esp_err_to_name(wait_err));

        return wait_err;
    }

    ESP_LOGI(TAG, "UART TX: %.*s", bytes_written, data);

    return ESP_OK;
}

//==================================================
// ESTADO
//==================================================

bool uart_transport_is_initialized(void)
{
    return initialized;
}

//==================================================
// RECEPCION UART
//==================================================

static void uart_rx_task(void *arg)
{
    uint8_t rx_buffer[128];

    ESP_LOGI(TAG, "Tarea RX UART iniciada");

    while (1)
    {
        int bytes_read = uart_read_bytes(
            MAIM_UART_PORT, rx_buffer, sizeof(rx_buffer), pdMS_TO_TICKS(100));

        if (bytes_read > 0)
        {
            ESP_LOGI(
                TAG,
                "UART RX RAW (%d bytes): %.*s",
                bytes_read,
                bytes_read,
                (char *)rx_buffer);

            for (int i = 0; i < bytes_read; i++)
            {
                uart_protocol_process_char((char)rx_buffer[i]);
            }
        }
    }
}

esp_err_t uart_transport_start_rx(void)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "No se puede iniciar RX: UART no inicializado");

        return ESP_ERR_INVALID_STATE;
    }

    if (rx_started)
    {
        return ESP_OK;
    }

    BaseType_t result =
        xTaskCreate(uart_rx_task, "uart_rx", 3072, NULL, 6, NULL);

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "No se pudo crear tarea RX UART");

        return ESP_ERR_NO_MEM;
    }

    rx_started = true;

    ESP_LOGI(TAG, "Recepcion UART habilitada");

    return ESP_OK;
}