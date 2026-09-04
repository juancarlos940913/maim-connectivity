#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

//==================================================
// CONFIGURACIÓN
//==================================================

#define WIFI_MANAGER_MAX_RETRIES 10

//==================================================
// INICIALIZACIÓN
//==================================================

/**
 * @brief Inicializa WiFi en modo Station y comienza
 *        la conexión al Access Point.
 *
 * @param ssid Nombre de la red WiFi.
 * @param password Contraseña de la red WiFi.
 *
 * @return
 *      ESP_OK si la inicialización fue correcta.
 *      Código de error ESP-IDF en caso contrario.
*/
esp_err_t wifi_manager_init(
    const char *ssid,
    const char *password
);

//==================================================
// ESTADO
//==================================================

/**
 * @brief Indica si el ESP32 tiene conexión WiFi
 *        y ya obtuvo una dirección IP.
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Obtiene RSSI de la conexión actual.
 *
 * @return RSSI en dBm.
 *         Devuelve 0 si no existe conexión.
 */
int8_t wifi_manager_get_rssi(void);

/**
 * @brief Obtiene la dirección IPv4 actual
 *        en formato texto.
 *
 * Ejemplo:
 * 192.168.1.120
 *
 * @param buffer Buffer de destino.
 * @param buffer_size Tamaño del buffer.
 *
 * @return ESP_OK si existe dirección válida.
 */
esp_err_t wifi_manager_get_ip(
    char *buffer,
    size_t buffer_size
);

/**
 * @brief Solicita una nueva conexión WiFi.
 */
esp_err_t wifi_manager_reconnect(void);

/**
 * @brief Desconecta WiFi.
 */
esp_err_t wifi_manager_disconnect(void);