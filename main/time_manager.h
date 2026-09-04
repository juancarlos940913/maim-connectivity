#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "esp_err.h"

//==================================================
// INICIALIZACION
//==================================================

/**
 * @brief Inicializa SNTP y sincroniza la hora
 *        utilizando servidores NTP.
 *
 * Debe llamarse despues de tener conexion WiFi.
 */
esp_err_t time_manager_init(void);

//==================================================
// ESTADO
//==================================================

/**
 * @brief Devuelve true cuando el reloj ya fue
 *        sincronizado correctamente.
 */
bool time_manager_is_synced(void);

//==================================================
// TIMESTAMP
//==================================================

/**
 * @brief Devuelve Unix timestamp UTC.
 *
 * @return segundos desde 1970.
 *         Devuelve 0 si aun no hay hora valida.
 */
int64_t time_manager_get_timestamp(void);

//==================================================
// HORA LOCAL
//==================================================

/**
 * @brief Obtiene fecha/hora local en texto.
 *
 * Ejemplo:
 * 2026-08-11 16:45:20
 */
esp_err_t time_manager_get_local_time(
    char *buffer,
    size_t buffer_size
);
