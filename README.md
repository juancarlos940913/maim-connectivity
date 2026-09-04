# MAIM Connectivity

Firmware de conectividad para equipos **MAIM**, desarrollado sobre **ESP32** utilizando **ESP-IDF**.

MAIM Connectivity funciona como una capa de comunicación entre el controlador principal del equipo —actualmente basado en ATmega— y la infraestructura de red MAIM.

La arquitectura busca ser reutilizable entre diferentes modelos de equipos, permitiendo que el controlador principal se concentre en el funcionamiento físico de la máquina mientras el ESP32 administra conectividad, comunicaciones remotas y servicios de red.

---

## 1. Arquitectura general

```text
                    SERVIDOR MAIM
                         │
                         │ MQTT
                         │
                    ┌────▼─────┐
                    │ Mosquitto│
                    └────┬─────┘
                         │
                    Wi-Fi / TCP-IP
                         │
                  ┌──────▼──────┐
                  │    ESP32    │
                  │             │
                  │ MAIM        │
                  │ Connectivity│
                  └──────┬──────┘
                         │
                         │ UART
                         │ MAIM Internal
                         │ UART Protocol v1
                         │
                  ┌──────▼──────┐
                  │    ATmega   │
                  │             │
                  │ Controlador │
                  │ principal   │
                  └──────┬──────┘
                         │
             ┌───────────┼───────────┐
             │           │           │
          Sensores   Actuadores   Interfaz
```

El **ATmega** mantiene el control directo del equipo:

- sensores;
- electroválvulas;
- bombas;
- botones;
- LEDs;
- lógica de dispensado;
- protecciones;
- procesos físicos del equipo.

El **ESP32** funciona como coprocesador de conectividad y administra:

- Wi-Fi;
- MQTT;
- sincronización de hora;
- comunicación con servidor;
- protocolo UART;
- interpretación del estado del controlador;
- eventos;
- comandos remotos;
- transacciones;
- timeouts;
- futuras actualizaciones OTA;
- futura actualización del firmware del controlador principal.

---

## 2. Objetivo del proyecto

El objetivo de **MAIM Connectivity** es establecer una arquitectura de comunicaciones común para diferentes equipos MAIM.

La intención es evitar desarrollar una solución de conectividad completamente diferente para cada modelo.

La estructura debe permitir que equipos con diferentes cantidades de:

- sensores;
- actuadores;
- funciones;
- métricas;
- errores;
- eventos;
- configuraciones;

puedan utilizar la misma arquitectura base.

---

## 3. Plataforma actual

### Hardware

- ESP32
- Flash: 4 MB
- Controlador principal previsto: ATmega
- Comunicación ESP32 ↔ controlador: UART

### Software

- ESP-IDF 5.5.5
- Visual Studio Code
- C
- FreeRTOS
- cJSON
- MQTT
- SNTP
- Git

### Servidor de desarrollo

- Rocky Linux
- Mosquitto MQTT Broker
- Máquina virtual
- Red NAT + adaptador puente

---

## 4. Estado actual del firmware

Actualmente se encuentran implementados y probados:

- [x] Inicialización ESP32
- [x] NVS
- [x] Wi-Fi Station
- [x] Reconexión Wi-Fi
- [x] Obtención de IP
- [x] Lectura RSSI
- [x] MQTT
- [x] Autenticación MQTT
- [x] Availability
- [x] Last Will / Offline
- [x] Telemetry
- [x] State Reported
- [x] Command Request
- [x] Command Response
- [x] Sincronización SNTP
- [x] Timestamp Unix
- [x] MAIM Internal UART Protocol v1
- [x] Device Manager
- [x] Eventos UART → MQTT inmediatos
- [x] Comandos MQTT → UART
- [x] Transaction Manager
- [x] ACK
- [x] DONE
- [x] NACK
- [x] ACK Timeout
- [x] Execution Timeout
- [x] Tabla de particiones OTA
- [x] Partición reservada para firmware AVR

Actualmente las respuestas del ATmega se están simulando manualmente mediante el monitor serie mientras se espera el hardware definitivo.

---

## 5. Estructura principal del firmware

```text
main/
│
├── main.c
│
├── maim_config.h
├── maim_secrets.h
├── maim_secrets.example.h
│
├── wifi_manager.c
├── wifi_manager.h
│
├── time_manager.c
├── time_manager.h
│
├── mqtt_manager.c
├── mqtt_manager.h
│
├── uart_protocol.c
├── uart_protocol.h
│
├── device_manager.c
├── device_manager.h
│
├── transaction_manager.c
└── transaction_manager.h
```

### `wifi_manager`

Responsable de:

- inicialización Wi-Fi;
- conexión;
- reconexión;
- dirección IP;
- RSSI;
- estado de conexión.

### `time_manager`

Responsable de:

- SNTP;
- sincronización de reloj;
- Unix timestamp.

### `mqtt_manager`

Responsable de:

- conexión al broker;
- topics;
- publicaciones;
- suscripciones;
- availability;
- telemetry;
- state reported;
- eventos;
- comandos;
- respuestas.

### `uart_protocol`

Implementa **MAIM Internal UART Protocol v1**.

Responsable de:

- recepción de tramas;
- validación;
- separación de campos;
- clasificación de mensajes;
- construcción de comandos.

### `device_manager`

Mantiene una representación del estado reportado por el controlador principal.

Administra información como:

- estados;
- métricas;
- sensores;
- outputs;
- errores;
- eventos.

### `transaction_manager`

Administra comandos enviados desde MQTT hacia el controlador principal.

Relaciona:

```text
MQTT command_id
        ↕
UART transaction_id
```

y administra:

- ACK;
- DONE;
- NACK;
- ACK Timeout;
- Execution Timeout.

---

## 6. Topics MQTT

La raíz actual es:

```text
maim/v1/devices/{device_id}/
```

Topics principales:

```text
maim/v1/devices/{device_id}/availability

maim/v1/devices/{device_id}/telemetry

maim/v1/devices/{device_id}/state/reported

maim/v1/devices/{device_id}/event

maim/v1/devices/{device_id}/command/request

maim/v1/devices/{device_id}/command/response
```

Ejemplo para el dispositivo de desarrollo:

```text
maim/v1/devices/MM_TEST_001/telemetry
```

---

## 7. Protocolo UART interno

El ESP32 y el controlador principal utilizan:

**MAIM Internal UART Protocol v1**

Formato general:

```text
<TYPE,FIELD1,FIELD2,...>
```

Ejemplos:

```text
<STATE,MODE,STANDBY>

<STATE,BUSY,0>

<METRIC,WATER_TOTAL_ML,16000>

<METRIC,LAST_DISPENSE_ML,287>

<METRIC,DISPENSE_COUNT,48>

<SENSOR,COLD_TANK_C,7.8>

<OUTPUT,VALVE,1>

<EVENT,DISPENSE_STARTED>

<EVENT,DISPENSE_COMPLETED,287,16287>

<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

Para transacciones:

```text
<CMD,15,DISPENSE>

<ACK,15>

<DONE,15>

<NACK,15,BUSY>
```

La especificación completa se encuentra dentro de `/docs`.

---

## 8. Flujo ATmega → servidor

Ejemplo de evento:

```text
ATmega
   │
   │ <EVENT,DISPENSE_COMPLETED,287,16287>
   ▼
UART Protocol
   │
   ▼
Device Manager
   │
   ▼
MQTT Manager
   │
   ▼
Mosquitto
   │
   ▼
Servidor MAIM
```

Los eventos importantes se publican inmediatamente y no necesitan esperar el ciclo periódico de telemetría.

---

## 9. Flujo servidor → ATmega

Ejemplo:

```text
Servidor
   │
   │ MQTT command/request
   ▼
Mosquitto
   │
   ▼
ESP32
   │
   ▼
Transaction Manager
   │
   │ <CMD,ID,DISPENSE>
   ▼
ATmega
```

El controlador responde:

```text
<ACK,ID>
```

y posteriormente:

```text
<DONE,ID>
```

El ESP32 convierte estas respuestas en:

```text
command/response → RECEIVED
```

y posteriormente:

```text
command/response → SUCCESS
```

Si el controlador rechaza la operación:

```text
<NACK,ID,BUSY>
```

se genera:

```text
command/response → REJECTED
```

---

## 10. Protección mediante timeouts

Las transacciones no pueden permanecer abiertas indefinidamente.

Actualmente existen dos mecanismos de protección.

### ACK Timeout

Tiempo actual:

```text
5 segundos
```

Si después de enviar:

```text
<CMD,ID,...>
```

el controlador no responde con:

```text
<ACK,ID>
```

la transacción termina como:

```text
FAILED
ACK_TIMEOUT
```

### Execution Timeout

Tiempo actual:

```text
30 segundos
```

Si el controlador responde:

```text
<ACK,ID>
```

pero nunca termina con:

```text
<DONE,ID>
```

la transacción termina como:

```text
FAILED
EXECUTION_TIMEOUT
```

Los timeouts utilizan un reloj monotónico interno y son independientes de la sincronización SNTP.

---

## 11. Particiones ESP32

El proyecto utiliza una tabla personalizada para flash de 4 MB.

```text
# Name,   Type, SubType, Offset,   Size

nvs,      data, nvs,     0x9000,   24K
otadata,  data, ota,     0xf000,   8K
phy_init, data, phy,     0x11000,  4K
ota_0,    app,  ota_0,   0x20000,  1536K
ota_1,    app,  ota_1,   0x1a0000, 1536K
avr_fw,   data, 0x40,    0x320000, 832K
```

Esto permite disponer de:

- dos particiones OTA para ESP32;
- espacio NVS;
- datos PHY;
- almacenamiento dedicado para futuro firmware del controlador AVR.

---

## 12. Compilación

El proyecto utiliza ESP-IDF.

Activar primero el entorno ESP-IDF correspondiente.

Ejemplo:

```powershell
& 'C:\Espressif\tools\Microsoft.v5.5.5.PowerShell_profile.ps1'
```

Compilar:

```powershell
idf.py build
```

Flashear:

```powershell
idf.py -p COM6 flash
```

Flashear y abrir monitor:

```powershell
idf.py -p COM6 flash monitor
```

El puerto COM puede cambiar dependiendo del equipo utilizado.

---

## 13. Credenciales

Las credenciales reales **NO deben almacenarse en Git**.

El archivo:

```text
main/maim_secrets.h
```

es local y está excluido mediante `.gitignore`.

Para configurar una nueva instalación:

1. Copiar:

```text
main/maim_secrets.example.h
```

2. Renombrar la copia como:

```text
main/maim_secrets.h
```

3. Configurar:

```c
#define MAIM_WIFI_SSID        "..."
#define MAIM_WIFI_PASSWORD    "..."

#define MAIM_MQTT_USER        "..."
#define MAIM_MQTT_PASSWORD    "..."
```

Nunca realizar commit de `maim_secrets.h`.

---

## 14. Documentación técnica

La documentación detallada se encuentra en:

```text
/docs
```

Comenzar por:

```text
docs/00_INDICE.md
```

La documentación incluye:

- arquitectura;
- instalación del servidor;
- Rocky Linux;
- Mosquitto;
- ESP-IDF;
- tabla de particiones;
- Wi-Fi;
- SNTP;
- MQTT;
- protocolo UART;
- Device Manager;
- eventos;
- comandos;
- transacciones;
- timeouts;
- procedimientos de prueba;
- diagnóstico.

---

## 15. Estado de desarrollo

Versión inicial de arquitectura:

```text
MAIM Connectivity v0.1.0
```

Esta versión representa la primera arquitectura funcional de comunicaciones.

Actualmente se encuentra validada mediante:

- ESP32 físico;
- broker Mosquitto;
- máquina virtual Rocky Linux;
- cliente Mosquitto Windows;
- simulación manual del controlador ATmega mediante terminal serie.

El siguiente objetivo es continuar desarrollando y validando la arquitectura antes de sustituir la simulación UART por comunicación física con el controlador principal.

---

## 16. Principio de diseño

MAIM Connectivity debe mantenerse:

- modular;
- independiente del modelo de equipo;
- extensible;
- versionable;
- diagnosticable;
- tolerante a fallos;
- compatible con futuras funciones;
- desacoplado de la lógica física del controlador principal.

Los cambios al protocolo deben mantener compatibilidad siempre que sea posible y documentarse junto con el firmware.