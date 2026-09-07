#include "uart_transport.h"

#include <stdbool.h>

#include "driver/uart.h"
#include "esp_log.h"

//==================================================
// LOG
//==================================================

static const char *TAG = "UART_TRANSPORT";

//==================================================
// CONFIGURACION
//==================================================

#define MAIM_UART_PORT        UART_NUM_2

#define MAIM_UART_TX_PIN      16
#define MAIM_UART_RX_PIN      17

#define MAIM_UART_BAUD_RATE   115200

#define MAIM_UART_RX_BUFFER_SIZE  512

//==================================================
// ESTADO
//==================================================

static bool initialized = false;

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

    const uart_config_t uart_config =
    {
        .baud_rate = MAIM_UART_BAUD_RATE,

        .data_bits =
            UART_DATA_8_BITS,

        .parity =
            UART_PARITY_DISABLE,

        .stop_bits =
            UART_STOP_BITS_1,

        .flow_ctrl =
            UART_HW_FLOWCTRL_DISABLE,

        .source_clk =
            UART_SCLK_DEFAULT
    };

    //--------------------------------------------------
    // APLICAR CONFIGURACION
    //--------------------------------------------------

    esp_err_t err =
        uart_param_config(
            MAIM_UART_PORT,
            &uart_config
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Error configurando UART: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    //--------------------------------------------------
    // ASIGNAR PINES
    //--------------------------------------------------

    err =
        uart_set_pin(
            MAIM_UART_PORT,
            MAIM_UART_TX_PIN,
            MAIM_UART_RX_PIN,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Error configurando pines UART: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    //--------------------------------------------------
    // INSTALAR DRIVER
    //
    // RX buffer habilitado.
    // TX buffer = 0 por ahora.
    // Sin queue de eventos en este bloque.
    //--------------------------------------------------

    err =
        uart_driver_install(
            MAIM_UART_PORT,
            MAIM_UART_RX_BUFFER_SIZE,
            0,
            0,
            NULL,
            0
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Error instalando driver UART: %s",
            esp_err_to_name(err)
        );

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
        MAIM_UART_BAUD_RATE
    );

    return ESP_OK;
}

//==================================================
// ESTADO
//==================================================

bool uart_transport_is_initialized(void)
{
    return initialized;
}