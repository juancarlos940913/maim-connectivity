#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#include "uart_protocol.h"

//==================================================
// LIMITES INTERNOS
//==================================================

#define DEVICE_MANAGER_MAX_STATES 16
#define DEVICE_MANAGER_MAX_METRICS 24
#define DEVICE_MANAGER_MAX_SENSORS 32
#define DEVICE_MANAGER_MAX_OUTPUTS 24
#define DEVICE_MANAGER_MAX_ERRORS 16

#define DEVICE_MANAGER_KEY_LEN 32
#define DEVICE_MANAGER_VALUE_LEN 40

#define DEVICE_MANAGER_MCU_LEN 24
#define DEVICE_MANAGER_MODEL_LEN 32
#define DEVICE_MANAGER_VERSION_LEN 16

//==================================================
// ENTRADA GENERICA KEY / VALUE
//==================================================

typedef struct
{
    bool used;

    char key[DEVICE_MANAGER_KEY_LEN];
    char value[DEVICE_MANAGER_VALUE_LEN];

} device_value_entry_t;

//==================================================
// ERROR ACTIVO
//==================================================

typedef struct
{
    bool used;

    char code[DEVICE_MANAGER_KEY_LEN];
    char severity[16];

} device_error_entry_t;

//==================================================
// IDENTIDAD DEL CONTROLADOR
//==================================================

typedef struct
{
    bool valid;

    char mcu[DEVICE_MANAGER_MCU_LEN];
    char model[DEVICE_MANAGER_MODEL_LEN];

    char hw_rev[DEVICE_MANAGER_VERSION_LEN];
    char fw_version[DEVICE_MANAGER_VERSION_LEN];

} device_controller_info_t;

//==================================================
// CALLBACK DE EVENTOS
//==================================================

typedef void (*device_manager_event_callback_t)(
    const char *event_type, const uart_protocol_frame_t *frame);

//==================================================
// CALLBACK DE TRANSACCIONES
//==================================================

typedef void (*device_manager_transaction_callback_t)(
    uart_frame_type_t type,
    uint16_t transaction_id,
    const uart_protocol_frame_t *frame);

//==================================================
// INICIALIZACION
//==================================================

void device_manager_init(void);

//==================================================
// PROCESAMIENTO
//==================================================

esp_err_t device_manager_process_frame(const uart_protocol_frame_t *frame);

//==================================================
// CALLBACKS
//==================================================

void device_manager_set_event_callback(
    device_manager_event_callback_t callback);

void device_manager_set_transaction_callback(
    device_manager_transaction_callback_t callback);

//==================================================
// ESTADO DE SNAPSHOT
//==================================================

bool device_manager_snapshot_active(void);

uint16_t device_manager_snapshot_id(void);

//==================================================
// IDENTIDAD
//==================================================

const device_controller_info_t *device_manager_get_controller_info(void);

//==================================================
// GETTERS GENERICOS
//==================================================

const char *device_manager_get_state(const char *key);

const char *device_manager_get_metric(const char *key);

const char *device_manager_get_sensor(const char *key);

const char *device_manager_get_output(const char *key);

//==================================================
// GETTERS CON CONVERSION
//==================================================

esp_err_t device_manager_get_metric_int64(const char *key, int64_t *value);

esp_err_t device_manager_get_sensor_float(const char *key, float *value);

esp_err_t device_manager_get_output_bool(const char *key, bool *value);

//==================================================
// ERRORES
//==================================================

size_t device_manager_get_error_count(void);

const device_error_entry_t *device_manager_get_error(size_t index);

//==================================================
// DEBUG
//==================================================

void device_manager_print_status(void);