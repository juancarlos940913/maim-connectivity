# MAIM Connectivity
# Índice Maestro de Documentación Técnica

**Proyecto:** MAIM Connectivity  
**Plataforma:** ESP32 / ESP-IDF  
**Versión inicial de arquitectura:** v0.1.0  
**Protocolo MQTT:** MAIM MQTT v1  
**Protocolo interno:** MAIM Internal UART Protocol v1  

---

## 1. Propósito de esta documentación

Esta carpeta contiene la documentación técnica del proyecto **MAIM Connectivity**.

Su objetivo es mantener documentados de forma reproducible:

- arquitectura del sistema;
- infraestructura utilizada;
- instalación y configuración del servidor;
- configuración del entorno de desarrollo;
- firmware ESP32;
- protocolos de comunicación;
- estructura MQTT;
- comunicación ESP32 ↔ controlador principal;
- eventos;
- comandos;
- transacciones;
- mecanismos de protección;
- procedimientos de prueba;
- diagnóstico de fallas;
- decisiones técnicas relevantes.

La documentación debe permitir que una persona con los conocimientos técnicos necesarios pueda reconstruir el entorno de desarrollo y comprender el funcionamiento del sistema sin depender del historial de documentación utilizada durante su desarrollo.

---

# 2. Filosofía documental

La documentación de MAIM Connectivity se divide en cuatro grandes bloques:

```text
INFRAESTRUCTURA
      │
      ▼
ENTORNO DE DESARROLLO
      │
      ▼
ARQUITECTURA Y PROTOCOLOS
      │
      ▼
PRUEBAS Y DIAGNÓSTICO
```

Cada documento debe concentrarse en un tema específico.

Cuando se agregue una nueva función importante al firmware, deberá determinarse si:

1. modifica un documento existente;
2. requiere una nueva sección;
3. requiere un nuevo documento;
4. modifica alguno de los protocolos;
5. modifica los procedimientos de prueba.

---

# 3. Índice general

## 01 — Arquitectura general

**Archivo:**

```text
01_ARQUITECTURA_GENERAL.md
```

Describe la arquitectura completa de MAIM Connectivity.

Incluye:

- propósito del sistema;
- división de responsabilidades;
- servidor;
- MQTT;
- ESP32;
- controlador principal;
- comunicación UART;
- flujo de información;
- principios de diseño;
- modularidad;
- escalabilidad hacia otros modelos MAIM.

Este documento responde principalmente:

> ¿Cómo está diseñado MAIM Connectivity?

---

## 02 — Servidor Rocky Linux

**Archivo:**

```text
02_SERVIDOR_ROCKY_LINUX.md
```

Documenta la creación y configuración del servidor de desarrollo.

Incluye:

- máquina virtual;
- instalación de Rocky Linux;
- configuración de red;
- NAT;
- adaptador puente;
- direccionamiento IP;
- conectividad;
- DNS;
- actualización del sistema;
- zona horaria;
- SSH;
- firewall;
- comandos administrativos;
- verificación del servidor.

Debe permitir reconstruir el servidor desde una instalación nueva.

Este documento responde:

> ¿Cómo reconstruyo el servidor utilizado por MAIM Connectivity?

---

## 03 — MQTT / Mosquitto

**Archivo:**

```text
03_MQTT_MOSQUITTO.md
```

Documenta el broker MQTT.

Incluye:

- instalación de Mosquitto;
- configuración;
- servicio `systemd`;
- puerto 1883;
- autenticación;
- archivo de contraseñas;
- permisos;
- configuración `maim.conf`;
- `include_dir`;
- inicio automático;
- publicación;
- suscripción;
- pruebas locales;
- pruebas desde Windows;
- diagnóstico.

También contiene los comandos operativos de Mosquitto utilizados durante el desarrollo.

Este documento responde:

> ¿Cómo funciona y cómo pruebo el broker MQTT?

---

## 04 — Entorno ESP-IDF

**Archivo:**

```text
04_ESP_IDF_ENTORNO.md
```

Documenta el entorno utilizado para desarrollar el firmware.

Incluye:

- Visual Studio Code;
- ESP-IDF Extension;
- ESP-IDF 5.5.5;
- PowerShell;
- activación del entorno;
- estructura inicial del proyecto;
- CMake;
- compilación;
- flash;
- monitor serie;
- comandos `idf.py`;
- solución de problemas comunes.

Este documento responde:

> ¿Cómo preparo una computadora para desarrollar MAIM Connectivity?

---

## 05 — Particiones ESP32

**Archivo:**

```text
05_PARTICIONES_ESP32.md
```

Documenta la memoria Flash del ESP32.

Incluye:

- Flash de 4 MB;
- tabla `partitions.csv`;
- NVS;
- OTA Data;
- PHY;
- `ota_0`;
- `ota_1`;
- `avr_fw`;
- offsets;
- tamaños;
- razón de diseño;
- futura actualización OTA;
- almacenamiento de firmware AVR.

Este documento responde:

> ¿Cómo está distribuida la memoria Flash y por qué?

---

## 06 — Wi-Fi

**Archivo:**

```text
06_WIFI.md
```

Documenta `wifi_manager`.

Incluye:

- modo Station;
- inicialización;
- eventos Wi-Fi;
- conexión;
- reconexión;
- dirección IP;
- gateway;
- máscara;
- RSSI;
- manejo de credenciales;
- comportamiento ante pérdida de conexión.

Este documento responde:

> ¿Cómo se conecta el ESP32 a la red?

---

## 07 — Sincronización de hora

**Archivo:**

```text
07_SINCRONIZACION_HORA.md
```

Documenta `time_manager`.

Incluye:

- SNTP;
- sincronización;
- timestamp Unix;
- inicialización;
- errores SNTP;
- servidores;
- dependencia de Wi-Fi;
- uso del timestamp en MQTT;
- diferencia entre hora de calendario y temporización monotónica.

Este documento responde:

> ¿De dónde obtiene el ESP32 la hora utilizada en los mensajes?

---

## 08 — Protocolo MQTT

**Archivo:**

```text
08_PROTOCOLO_MQTT.md
```

Especificación del protocolo MQTT utilizado por MAIM.

Incluye:

- versión del esquema;
- estructura raíz;
- Device ID;
- topics;
- QoS;
- retain;
- availability;
- telemetry;
- state/reported;
- event;
- command/request;
- command/response;
- payloads;
- timestamps;
- errores;
- convenciones;
- ejemplos completos.

Este documento debe tratarse como una **especificación de protocolo**.

Este documento responde:

> ¿Cómo se comunica un equipo MAIM con el servidor?

---

## 09 — Protocolo UART interno

**Archivo:**

```text
09_PROTOCOLO_UART_INTERNO.md
```

Especificación de:

**MAIM Internal UART Protocol v1**

Incluye:

- formato de trama;
- delimitadores;
- tipos de mensajes;
- campos;
- STATE;
- METRIC;
- SENSOR;
- OUTPUT;
- EVENT;
- ERROR;
- CMD;
- ACK;
- DONE;
- NACK;
- validación;
- ejemplos;
- reglas de compatibilidad.

Este documento debe tratarse como una **especificación formal de comunicación interna**.

Este documento responde:

> ¿Cómo se comunican el ESP32 y el controlador principal?

---

## 10 — Device Manager

**Archivo:**

```text
10_DEVICE_MANAGER.md
```

Documenta la representación interna del estado del equipo.

Incluye:

- función de `device_manager`;
- procesamiento de tramas;
- estados;
- métricas;
- sensores;
- outputs;
- errores;
- callbacks;
- relación con UART;
- relación con MQTT;
- construcción de `state/reported`.

Este documento responde:

> ¿Cómo convierte el ESP32 las tramas del controlador en información estructurada?

---

## 11 — Eventos

**Archivo:**

```text
11_EVENTOS.md
```

Documenta el sistema de eventos inmediatos.

Incluye inicialmente:

- `DISPENSE_STARTED`;
- `DISPENSE_COMPLETED`;
- `DISPENSE_CANCELLED`;
- `CALIBRATION_COMPLETED`;
- `PROGRAMMING_CHANGED`;
- Event ID;
- timestamp;
- severity;
- data;
- eventos genéricos;
- publicación inmediata MQTT.

Describe la ruta:

```text
Controlador
    ↓
UART EVENT
    ↓
ESP32
    ↓
MQTT EVENT
    ↓
Servidor
```

Este documento responde:

> ¿Cómo informa el equipo inmediatamente que ocurrió algo?

---

## 12 — Comandos y transacciones

**Archivo:**

```text
12_COMANDOS_Y_TRANSACCIONES.md
```

Documenta la comunicación servidor → controlador.

Incluye:

- `command/request`;
- `command_id`;
- comandos internos del ESP32;
- comandos dirigidos al controlador;
- Transaction Manager;
- UART Transaction ID;
- relación entre IDs;
- CMD;
- ACK;
- DONE;
- NACK;
- RECEIVED;
- SUCCESS;
- REJECTED;
- FAILED;
- ciclo de vida de una transacción.

Describe la ruta:

```text
Servidor
   ↓
MQTT
   ↓
ESP32
   ↓
Transaction Manager
   ↓
UART CMD
   ↓
Controlador
```

y su respuesta.

Este documento responde:

> ¿Cómo ejecuta el servidor una acción en el equipo y cómo sabe si terminó?

---

## 13 — Timeouts

**Archivo:**

```text
13_TIMEOUTS.md
```

Documenta los mecanismos que evitan transacciones bloqueadas.

Incluye:

### ACK Timeout

Tiempo inicial:

```text
5000 ms
```

Detecta cuando el controlador no responde a un comando.

Resultado:

```text
FAILED / ACK_TIMEOUT
```

### Execution Timeout

Tiempo inicial:

```text
30000 ms
```

Detecta cuando el controlador confirmó el comando pero nunca terminó la operación.

Resultado:

```text
FAILED / EXECUTION_TIMEOUT
```

También documenta:

- reloj monotónico;
- `esp_timer_get_time()`;
- tarea supervisora;
- liberación de transacciones;
- comportamiento ante respuestas tardías;
- futura configuración específica por comando.

Este documento responde:

> ¿Qué ocurre si el controlador deja de responder?

---

## 14 — Pruebas y diagnóstico

**Archivo:**

```text
14_PRUEBAS_Y_DIAGNOSTICO.md
```

Documento operativo para desarrollo, pruebas y servicio.

Incluye comandos exactos para:

- comprobar servidor;
- comprobar red;
- comprobar Mosquitto;
- escuchar topics;
- escuchar un dispositivo;
- publicar desde Linux;
- publicar desde PowerShell;
- solicitar `GET_STATE`;
- enviar `DISPENSE`;
- simular UART;
- simular eventos;
- simular ACK;
- simular DONE;
- simular NACK;
- provocar ACK Timeout;
- provocar Execution Timeout;
- comprobar timestamps;
- comprobar RSSI;
- diagnosticar errores.

Debe indicar para cada prueba:

```text
OBJETIVO
COMANDO
RESULTADO ESPERADO
FALLA POSIBLE
DIAGNÓSTICO
```

Este documento responde:

> ¿Cómo compruebo rápidamente que cada parte del sistema funciona?

---

# 4. Documentos futuros previstos

La arquitectura está diseñada para crecer.

Conforme se implementen nuevas funciones podrán añadirse documentos como:

```text
15_OTA_ESP32.md
16_ACTUALIZACION_AVR.md
17_PROVISIONING.md
18_SEGURIDAD.md
19_LOGS_Y_DIAGNOSTICO_REMOTO.md
20_BACKEND_MAIM.md
21_BASE_DE_DATOS.md
22_API_SERVIDOR.md
23_MODELOS_Y_CAPABILITIES.md
24_VERSIONADO_PROTOCOLOS.md
25_PRODUCCION_Y_PROVISIONING.md
```

Estos documentos **no deben crearse únicamente para llenar la estructura**.

Se crearán cuando la función correspondiente sea diseñada o implementada.

---

# 5. Clasificación de la documentación

Para mantener organizada la documentación se utilizará la siguiente clasificación conceptual.

## Infraestructura

```text
02_SERVIDOR_ROCKY_LINUX
03_MQTT_MOSQUITTO
```

## Desarrollo ESP32

```text
04_ESP_IDF_ENTORNO
05_PARTICIONES_ESP32
06_WIFI
07_SINCRONIZACION_HORA
```

## Protocolos y arquitectura

```text
01_ARQUITECTURA_GENERAL
08_PROTOCOLO_MQTT
09_PROTOCOLO_UART_INTERNO
10_DEVICE_MANAGER
11_EVENTOS
12_COMANDOS_Y_TRANSACCIONES
13_TIMEOUTS
```

## Operación y validación

```text
14_PRUEBAS_Y_DIAGNOSTICO
```

---

# 6. Convenciones

## 6.1 Credenciales

Nunca deben escribirse credenciales reales dentro de esta documentación.

Utilizar siempre:

```text
<WIFI_SSID>
<WIFI_PASSWORD>

<MQTT_HOST>
<MQTT_PORT>
<MQTT_USER>
<MQTT_PASSWORD>

<DEVICE_ID>
```

Ejemplo:

```powershell
mosquitto_sub -h <MQTT_HOST> -p <MQTT_PORT> -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/<DEVICE_ID>/#" -v
```

---

## 6.2 Comandos

Todos los comandos deben indicar en qué entorno deben ejecutarse.

Ejemplo:

### Rocky Linux

```bash
sudo systemctl status mosquitto
```

### Windows PowerShell

```powershell
idf.py build
```

### Monitor UART

```text
<EVENT,DISPENSE_STARTED>
```

Esto evita ejecutar comandos en el entorno incorrecto.

---

## 6.3 Resultados esperados

Siempre que sea posible, una prueba debe mostrar el resultado esperado.

Ejemplo:

```text
Entrada:

<ACK,15>

Resultado:

command/response
status = RECEIVED
```

---

## 6.4 Valores configurables

Los valores que puedan cambiar entre instalaciones no deben considerarse constantes universales.

Ejemplos:

```text
IP del broker
puerto COM
SSID
contraseña
Device ID
direcciones IP
```

La documentación puede mostrar los valores utilizados durante desarrollo, pero debe identificarlos claramente como valores de laboratorio.

---

# 7. Versionado

El firmware y la documentación deben evolucionar juntos.

Cuando un cambio modifique:

- protocolo UART;
- protocolo MQTT;
- estructura JSON;
- comportamiento de comandos;
- eventos;
- timeouts;
- particiones;
- arquitectura;

debe actualizarse también la documentación correspondiente.

No debe considerarse terminada una nueva función hasta que:

```text
IMPLEMENTACIÓN
      ↓
COMPILACIÓN
      ↓
PRUEBA
      ↓
VALIDACIÓN
      ↓
DOCUMENTACIÓN
      ↓
COMMIT
```

---

# 8. Estados de implementación

Dentro de la documentación podrán utilizarse las siguientes etiquetas:

```text
[IMPLEMENTADO]
[PROBADO]
[EN DESARROLLO]
[PLANEADO]
[DEPRECADO]
```

### IMPLEMENTADO

Existe código funcional para la característica.

### PROBADO

La característica fue comprobada mediante una prueba documentada.

### EN DESARROLLO

La implementación existe parcialmente y puede cambiar.

### PLANEADO

Forma parte de la arquitectura prevista, pero todavía no debe considerarse disponible.

### DEPRECADO

La característica se conserva por compatibilidad o historial, pero no debe utilizarse en nuevos desarrollos.

---

# 9. Estado documental actual

Para la arquitectura inicial **v0.1.0**, se documentarán las funciones que ya han sido implementadas y probadas:

```text
[PROBADO] Wi-Fi
[PROBADO] MQTT
[PROBADO] Availability
[PROBADO] Telemetry
[PROBADO] State Reported
[PROBADO] SNTP
[PROBADO] Timestamp Unix
[PROBADO] UART Protocol v1
[PROBADO] Device Manager
[PROBADO] Eventos inmediatos
[PROBADO] Command Request
[PROBADO] Command Response
[PROBADO] Transaction Manager
[PROBADO] ACK
[PROBADO] DONE
[PROBADO] NACK
[PROBADO] ACK Timeout
[PROBADO] Execution Timeout

[PLANEADO] UART físico con controlador definitivo
[PLANEADO] OTA ESP32
[PLANEADO] Actualización firmware AVR
[PLANEADO] Provisioning de equipos
[PLANEADO] Seguridad de producción
```

---

# 10. Orden recomendado de lectura

Para comprender el proyecto completo:

```text
README.md
   ↓
00_INDICE.md
   ↓
01_ARQUITECTURA_GENERAL.md
   ↓
08_PROTOCOLO_MQTT.md
   ↓
09_PROTOCOLO_UART_INTERNO.md
   ↓
10_DEVICE_MANAGER.md
   ↓
11_EVENTOS.md
   ↓
12_COMANDOS_Y_TRANSACCIONES.md
   ↓
13_TIMEOUTS.md
```

Para reconstruir el entorno:

```text
02_SERVIDOR_ROCKY_LINUX.md
   ↓
03_MQTT_MOSQUITTO.md
   ↓
04_ESP_IDF_ENTORNO.md
   ↓
05_PARTICIONES_ESP32.md
   ↓
06_WIFI.md
   ↓
07_SINCRONIZACION_HORA.md
```

Para realizar pruebas:

```text
14_PRUEBAS_Y_DIAGNOSTICO.md
```

---

# 11. Regla principal

La documentación debe describir **el sistema que realmente existe y ha sido validado**, diferenciándolo claramente de las funciones previstas para el futuro.

No se documentará una función planeada como si ya estuviera implementada.

El objetivo es que la documentación sea una fuente técnica confiable del estado real de **MAIM Connectivity**.