#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "uart_protocol.h"

//==================================================
// INICIALIZACIÓN
//==================================================

/**
 * @brief Inicializa el cliente MQTT MAIM.
 *
 * Debe llamarse después de inicializar WiFi.
 */
esp_err_t mqtt_manager_init(void);

//==================================================
// ESTADO
//==================================================

/**
 * @brief Indica si actualmente existe conexión
 *        con el broker MQTT.
 */
bool mqtt_manager_is_connected(void);

//==================================================
// PUBLICACIONES MAIM
//==================================================

/**
 * @brief Publica availability = online.
 */
esp_err_t mqtt_manager_publish_online(void);

/**
 * @brief Publica availability = sleeping.
 */
esp_err_t mqtt_manager_publish_sleeping(uint32_t next_wakeup_sec);

/**
 * @brief Publica el estado general del dispositivo.
 */
esp_err_t mqtt_manager_publish_state(void);

/**
 * @brief Publica telemetría básica.
 */
esp_err_t mqtt_manager_publish_telemetry(void);

esp_err_t mqtt_manager_publish_event(
    const char *event_type, const uart_protocol_frame_t *frame);

esp_err_t mqtt_manager_publish_transaction_response(
    const char *command_id, const char *status, const char *reason);