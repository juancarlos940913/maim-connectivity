#include "time_manager.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_netif_sntp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//==================================================
// LOG
//==================================================

static const char *TAG = "TIME_MANAGER";

//==================================================
// ESTADO
//==================================================

static bool time_synced = false;

//==================================================
// VALIDAR HORA
//==================================================

static bool time_is_valid(void)
{
    time_t now = time(NULL);

    /*
     * Umbral sencillo:
     * si estamos despues de 2024,
     * consideramos que el reloj es valido.
     */
    return now > 1704067200;
}

//==================================================
// INICIALIZAR
//==================================================

esp_err_t time_manager_init(void)
{
    if (time_synced)
    {
        return ESP_OK;
    }

    ESP_LOGI(
        TAG,
        "Inicializando sincronizacion SNTP"
    );

    //--------------------------------------------------
    // ZONA HORARIA
    //
    // IMPORTANTE:
    // MQTT usa timestamps Unix UTC.
    // Esta zona solo se utiliza cuando queremos
    // imprimir hora local.
    //
    // Mexico central actualmente UTC-6 sin DST.
    //--------------------------------------------------

    setenv(
        "TZ",
        "CST6",
        1
    );

    tzset();

    //--------------------------------------------------
    // CONFIGURACION SNTP
    //--------------------------------------------------

    esp_sntp_config_t config =
        ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(
            2,
            ESP_SNTP_SERVER_LIST(
                "pool.ntp.org"
            )
        );

    //--------------------------------------------------
    // INICIALIZAR
    //--------------------------------------------------

    esp_err_t err =
        esp_netif_sntp_init(
            &config
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Error inicializando SNTP: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    ESP_LOGI(
        TAG,
        "Esperando sincronizacion de hora..."
    );

    //--------------------------------------------------
    // ESPERAR MAXIMO 15 SEGUNDOS
    //--------------------------------------------------

    err =
        esp_netif_sntp_sync_wait(
            pdMS_TO_TICKS(15000)
        );

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "No se obtuvo hora SNTP dentro del tiempo esperado"
        );

        return err;
    }

    //--------------------------------------------------
    // VALIDAR
    //--------------------------------------------------

    if (!time_is_valid())
    {
        ESP_LOGE(
            TAG,
            "SNTP respondio pero la hora no es valida"
        );

        return ESP_FAIL;
    }

    time_synced = true;

    //--------------------------------------------------
    // MOSTRAR HORA
    //--------------------------------------------------

    char local_time[32];

    if (
        time_manager_get_local_time(
            local_time,
            sizeof(local_time)
        ) == ESP_OK
    )
    {
        ESP_LOGI(
            TAG,
            "Hora sincronizada: %s",
            local_time
        );
    }

    ESP_LOGI(
        TAG,
        "Unix timestamp: %lld",
        time_manager_get_timestamp()
    );

    return ESP_OK;
}

//==================================================
// ESTA SINCRONIZADO
//==================================================

bool time_manager_is_synced(void)
{
    if (!time_synced)
    {
        time_synced =
            time_is_valid();
    }

    return time_synced;
}

//==================================================
// TIMESTAMP UTC
//==================================================

int64_t time_manager_get_timestamp(void)
{
    if (!time_manager_is_synced())
    {
        return 0;
    }

    time_t now =
        time(NULL);

    return (int64_t)now;
}

//==================================================
// HORA LOCAL
//==================================================

esp_err_t time_manager_get_local_time(
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

    if (!time_manager_is_synced())
    {
        return ESP_ERR_INVALID_STATE;
    }

    time_t now =
        time(NULL);

    struct tm timeinfo;

    localtime_r(
        &now,
        &timeinfo
    );

    size_t result =
        strftime(
            buffer,
            buffer_size,
            "%Y-%m-%d %H:%M:%S",
            &timeinfo
        );

    if (result == 0)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}