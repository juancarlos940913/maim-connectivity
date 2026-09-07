idf_component_register(
    SRCS
        "main.c"
        "wifi_manager.c"
        "mqtt_manager.c"
        "time_manager.c"
        "uart_protocol.c"
        "device_manager.c"
        "transaction_manager.c"
        "uart_transport.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        mqtt
        json
        esp_driver_uart
)