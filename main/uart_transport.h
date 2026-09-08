#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

//==================================================
// MAIM UART TRANSPORT
//==================================================

esp_err_t uart_transport_init(void);

esp_err_t uart_transport_send(const char *data, size_t length);

esp_err_t uart_transport_start_rx(void);

bool uart_transport_is_initialized(void);