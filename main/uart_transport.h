#pragma once

#include <stdbool.h>

#include "esp_err.h"

//==================================================
// MAIM UART TRANSPORT
//==================================================

esp_err_t uart_transport_init(void);

bool uart_transport_is_initialized(void);