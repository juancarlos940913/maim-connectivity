#include "wifi_manager.h"

#include <stdio.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"

//==================================================
// LOG
//==================================================

static const char *TAG = "WIFI_MANAGER";

//==================================================
// VARIABLES INTERNAS
//==================================================

static bool wifi_connected = false;

static uint8_t retry_count = 0;

static esp_netif_t *wifi_netif = NULL;

static esp_event_handler_instance_t instance_wifi_event;
static esp_event_handler_instance_t instance_ip_event;

//==================================================
// EVENT HANDLER
//==================================================

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    //--------------------------------------------------
    // WIFI INICIADO
    //--------------------------------------------------

    if (
        event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START
    )
    {
        ESP_LOGI(TAG, "WiFi iniciado");

        esp_err_t err = esp_wifi_connect();

        if (err != ESP_OK)
        {
            ESP_LOGE(
                TAG,
                "Error al iniciar conexion WiFi: %s",
                esp_err_to_name(err)
            );
        }

        return;
    }

    //--------------------------------------------------
    // WIFI DESCONECTADO
    //--------------------------------------------------

    if (
        event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_DISCONNECTED
    )
    {
        wifi_connected = false;

        wifi_event_sta_disconnected_t *event =
            (wifi_event_sta_disconnected_t *)event_data;

        ESP_LOGW(
            TAG,
            "WiFi desconectado. Razon: %d",
            event->reason
        );

        if (retry_count < WIFI_MANAGER_MAX_RETRIES)
        {
            retry_count++;

            ESP_LOGI(
                TAG,
                "Reintentando conexion WiFi (%u/%u)",
                retry_count,
                WIFI_MANAGER_MAX_RETRIES
            );

            esp_err_t err = esp_wifi_connect();

            if (err != ESP_OK)
            {
                ESP_LOGE(
                    TAG,
                    "Error al reconectar: %s",
                    esp_err_to_name(err)
                );
            }
        }
        else
        {
            ESP_LOGE(
                TAG,
                "No fue posible conectar despues de %u intentos",
                WIFI_MANAGER_MAX_RETRIES
            );
        }

        return;
    }

    //--------------------------------------------------
    // IP OBTENIDA
    //--------------------------------------------------

    if (
        event_base == IP_EVENT &&
        event_id == IP_EVENT_STA_GOT_IP
    )
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        wifi_connected = true;
        retry_count = 0;

        ESP_LOGI(
            TAG,
            "WiFi conectado correctamente"
        );

        ESP_LOGI(
            TAG,
            "IP: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );

        ESP_LOGI(
            TAG,
            "Gateway: " IPSTR,
            IP2STR(&event->ip_info.gw)
        );

        ESP_LOGI(
            TAG,
            "Mascara: " IPSTR,
            IP2STR(&event->ip_info.netmask)
        );

        return;
    }
}

//==================================================
// INICIALIZAR WIFI
//==================================================

esp_err_t wifi_manager_init(
    const char *ssid,
    const char *password
)
{
    if (ssid == NULL || password == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Inicializando WiFi");

    //--------------------------------------------------
    // TCP/IP STACK
    //--------------------------------------------------

    esp_err_t err = esp_netif_init();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "esp_netif_init fallo: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    //--------------------------------------------------
    // EVENT LOOP
    //--------------------------------------------------

    err = esp_event_loop_create_default();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Event Loop fallo: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    //--------------------------------------------------
    // INTERFAZ WIFI STA
    //--------------------------------------------------

    wifi_netif =
        esp_netif_create_default_wifi_sta();

    if (wifi_netif == NULL)
    {
        ESP_LOGE(
            TAG,
            "No fue posible crear interfaz WiFi STA"
        );

        return ESP_FAIL;
    }

    //--------------------------------------------------
    // DRIVER WIFI
    //--------------------------------------------------

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(&cfg);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "esp_wifi_init fallo: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    //--------------------------------------------------
    // REGISTRAR EVENTOS WIFI
    //--------------------------------------------------

    err = esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        &instance_wifi_event
    );

    if (err != ESP_OK)
    {
        return err;
    }

    //--------------------------------------------------
    // REGISTRAR EVENTO IP
    //--------------------------------------------------

    err = esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        &instance_ip_event
    );

    if (err != ESP_OK)
    {
        return err;
    }

    //--------------------------------------------------
    // CONFIGURACIÓN WIFI
    //--------------------------------------------------

    wifi_config_t wifi_config = {0};

    strncpy(
        (char *)wifi_config.sta.ssid,
        ssid,
        sizeof(wifi_config.sta.ssid) - 1
    );

    strncpy(
        (char *)wifi_config.sta.password,
        password,
        sizeof(wifi_config.sta.password) - 1
    );

    //--------------------------------------------------
    // SEGURIDAD
    //--------------------------------------------------

    wifi_config.sta.threshold.authmode =
        WIFI_AUTH_WPA2_PSK;

    //--------------------------------------------------
    // MODO STATION
    //--------------------------------------------------

    err = esp_wifi_set_mode(WIFI_MODE_STA);

    if (err != ESP_OK)
    {
        return err;
    }

    //--------------------------------------------------
    // APLICAR CONFIGURACIÓN
    //--------------------------------------------------

    err = esp_wifi_set_config(
        WIFI_IF_STA,
        &wifi_config
    );

    if (err != ESP_OK)
    {
        return err;
    }

    //--------------------------------------------------
    // ARRANCAR WIFI
    //--------------------------------------------------

    err = esp_wifi_start();

    if (err != ESP_OK)
    {
        return err;
    }

    ESP_LOGI(
        TAG,
        "Intentando conectar a: %s",
        ssid
    );

    return ESP_OK;
}

//==================================================
// ¿ESTA CONECTADO?
//==================================================

bool wifi_manager_is_connected(void)
{
    return wifi_connected;
}

//==================================================
// RSSI
//==================================================

int8_t wifi_manager_get_rssi(void)
{
    if (!wifi_connected)
    {
        return 0;
    }

    wifi_ap_record_t ap_info = {0};

    esp_err_t err =
        esp_wifi_sta_get_ap_info(&ap_info);

    if (err != ESP_OK)
    {
        return 0;
    }

    return ap_info.rssi;
}

//==================================================
// OBTENER IP
//==================================================

esp_err_t wifi_manager_get_ip(
    char *buffer,
    size_t buffer_size
)
{
    if (
        buffer == NULL ||
        buffer_size == 0
    )
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!wifi_connected || wifi_netif == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_netif_ip_info_t ip_info;

    esp_err_t err =
        esp_netif_get_ip_info(
            wifi_netif,
            &ip_info
        );

    if (err != ESP_OK)
    {
        return err;
    }

    snprintf(
        buffer,
        buffer_size,
        IPSTR,
        IP2STR(&ip_info.ip)
    );

    return ESP_OK;
}

//==================================================
// RECONECTAR
//==================================================

esp_err_t wifi_manager_reconnect(void)
{
    retry_count = 0;

    ESP_LOGI(
        TAG,
        "Conexion WiFi solicitada"
    );

    return esp_wifi_connect();
}

//==================================================
// DESCONECTAR
//==================================================

esp_err_t wifi_manager_disconnect(void)
{
    wifi_connected = false;

    ESP_LOGI(
        TAG,
        "Desconexion WiFi solicitada"
    );

    return esp_wifi_disconnect();
}