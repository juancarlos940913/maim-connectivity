# MAIM Connectivity

## Protocolo MQTT v1

**Documento:** 08_PROTOCOLO_MQTT.md
**Proyecto:** MAIM Connectivity
**Protocolo:** MAIM MQTT Protocol v1
**Broker actual:** Eclipse Mosquitto 2.0.22
**Transporte actual:** MQTT TCP
**Puerto de laboratorio:** 1883
**Formato de payload:** JSON
**Estado:** [PROBADO] Protocolo base funcional

---

# 1. Propósito

Este documento define formalmente el protocolo MQTT utilizado por **MAIM Connectivity**.

El objetivo es establecer una estructura común para que diferentes equipos MAIM puedan comunicarse con la infraestructura utilizando el mismo modelo de:

* topics;
* identificación;
* telemetría;
* estado;
* eventos;
* comandos;
* respuestas;
* disponibilidad;
* timestamps;
* errores.

Este documento debe considerarse la referencia principal para cualquier implementación que publique o consuma mensajes MAIM MQTT v1.

---

# 2. Principio general

La comunicación sigue una estructura orientada a dispositivo.

La raíz actual es:

```text
maim/v1/devices/{device_id}/
```

Ejemplo:

```text
maim/v1/devices/MM_TEST_001/
```

Cada dispositivo tiene su propio espacio MQTT.

---

# 3. Identidad del dispositivo

Cada equipo debe disponer de un identificador único:

```text
device_id
```

Ejemplo:

```text
MM_TEST_001
```

El `device_id` se utiliza directamente dentro de los topics.

Ejemplo:

```text
maim/v1/devices/MM_TEST_001/telemetry
```

---

# 4. Regla de identidad

La identidad MQTT no debe depender de:

* dirección IP;
* MAC de red como identificador operativo principal;
* hostname temporal;
* IP DHCP;
* SSID.

La identidad lógica debe ser:

```text
MAIM_DEVICE_ID
```

---

# 5. Estructura general de topics

```text
maim/
└── v1/
    └── devices/
        └── {device_id}/
            │
            ├── availability
            │
            ├── telemetry
            │
            ├── state/
            │   └── reported
            │
            ├── event
            │
            └── command/
                ├── request
                └── response
```

---

# 6. Topics oficiales v1

Actualmente se definen:

```text
maim/v1/devices/{device_id}/availability
```

```text
maim/v1/devices/{device_id}/telemetry
```

```text
maim/v1/devices/{device_id}/state/reported
```

```text
maim/v1/devices/{device_id}/event
```

```text
maim/v1/devices/{device_id}/command/request
```

```text
maim/v1/devices/{device_id}/command/response
```

---

# 7. Versión del protocolo

La versión se encuentra explícitamente en el topic:

```text
v1
```

Ejemplo:

```text
maim/v1/devices/MM_TEST_001/event
```

Esto permite introducir en el futuro:

```text
maim/v2/...
```

si existiera un cambio incompatible.

---

# 8. Campo `schema`

Los payloads utilizan:

```json
"schema": 1
```

Ejemplo:

```json
{
  "schema": 1
}
```

El campo permite versionar la estructura JSON independientemente del topic.

---

# 9. Diferencia entre versión de topic y schema

Actualmente:

```text
Topic protocol version:
v1
```

y:

```text
JSON schema:
1
```

Ambos coinciden conceptualmente, pero deben considerarse mecanismos diferentes.

Un cambio menor del JSON podría evolucionar sin necesariamente cambiar toda la jerarquía MQTT.

---

# 10. Formato de datos

Los payloads MQTT utilizan:

```text
JSON UTF-8
```

Ejemplo:

```json
{
  "schema": 1,
  "ts": 1786497569
}
```

---

# 11. Timestamp

El campo temporal estándar es:

```text
ts
```

Formato:

```text
Unix timestamp
```

Unidad:

```text
segundos
```

Referencia:

```text
UTC
```

Ejemplo:

```json
"ts": 1786497569
```

---

# 12. Fuente del timestamp

El ESP32 obtiene la hora mediante:

```text
SNTP
```

a través de:

```text
time_manager
```

La implementación se documenta en:

```text
07_SINCRONIZACION_HORA.md
```

---

# 13. Timestamp no disponible

Si el ESP32 todavía no dispone de una hora válida:

```json
"ts": 0
```

puede utilizarse temporalmente.

Esto indica:

```text
TIME_NOT_SYNCED
```

conceptualmente.

No debe interpretarse como una fecha real.

---

# 14. Convenciones de nombres JSON

Las claves JSON utilizan:

```text
snake_case
```

Ejemplos:

```text
device_id
hw_rev
water_total_ml
last_dispense_ml
rssi_dbm
```

---

# 15. Convenciones de valores enumerados

Los valores de protocolo utilizados como estados o comandos se escriben normalmente en:

```text
UPPER_SNAKE_CASE
```

Ejemplos:

```text
STANDBY
DISPENSING
GET_STATE
DISPENSE
ACK_TIMEOUT
FLOW_SENSOR_FAIL
```

---

# 16. Unidades

Cuando sea posible, la unidad debe formar parte del nombre.

Ejemplos:

```text
water_total_ml
temperature_c
rssi_dbm
dispense_time_ms
runtime_sec
```

Esto evita ambigüedad.

---

# 17. Booleanos

MQTT JSON utiliza booleanos JSON reales:

```json
true
```

```json
false
```

No utilizar:

```json
"true"
```

como string salvo que exista una razón específica.

---

# 18. Números

Las mediciones deben enviarse como números JSON.

Correcto:

```json
"water_total_ml": 16000
```

No recomendado:

```json
"water_total_ml": "16000"
```

El protocolo UART puede transportar valores como texto, pero el ESP32 realiza la conversión antes de publicar MQTT.

---

# 19. `availability`

Topic:

```text
maim/v1/devices/{device_id}/availability
```

Dirección:

```text
DISPOSITIVO → BROKER
```

Propósito:

Indicar el estado de conectividad MQTT del dispositivo.

---

# 20. Availability Online

Cuando el ESP32 conecta al broker publica:

```json
{
  "schema": 1,
  "ts": 1786497569,
  "status": "online"
}
```

---

# 21. Availability Offline

En caso de desconexión inesperada, el Last Will genera conceptualmente:

```json
{
  "schema": 1,
  "status": "offline"
}
```

La implementación actual puede omitir `ts` en el Last Will porque el mensaje se prepara antes de conocer el momento exacto de desconexión.

---

# 22. Retain de availability

`availability` debe utilizar:

```text
retain = true
```

Esto permite que un cliente nuevo conozca inmediatamente el último estado conocido.

---

# 23. QoS availability

Recomendación actual:

```text
QoS 1
```

para reducir la probabilidad de pérdida del cambio de estado.

---

# 24. Last Will

El dispositivo registra el Will antes de conectarse.

Conceptualmente:

```text
Will topic:
maim/v1/devices/{device_id}/availability

Will payload:
status = offline

QoS:
1

Retain:
true
```

---

# 25. Tiempo de detección Offline

Durante laboratorio se observó aproximadamente:

```text
~3 minutos
```

con el keepalive normal de MQTT.

Se decidió mantener el comportamiento estándar del broker por el momento.

---

# 26. `telemetry`

Topic:

```text
maim/v1/devices/{device_id}/telemetry
```

Dirección:

```text
DISPOSITIVO → SERVIDOR
```

Propósito:

Enviar muestras periódicas destinadas principalmente a:

* históricos;
* tendencias;
* monitoreo;
* análisis.

---

# 27. Telemetría actual

Actualmente se publica información de red:

```json
{
  "schema": 1,
  "ts": 1786493729,
  "network": {
    "rssi_dbm": -54
  }
}
```

---

# 28. Periodicidad actual

Durante desarrollo se utiliza aproximadamente:

```text
30 segundos
```

para la publicación de telemetría.

Este intervalo puede cambiar posteriormente.

---

# 29. Retain de telemetry

Debe utilizar:

```text
retain = false
```

La telemetría representa muestras temporales y no un estado autoritativo actual.

---

# 30. QoS telemetry

Para telemetría periódica puede utilizarse:

```text
QoS 0
```

porque la pérdida ocasional de una muestra puede ser aceptable.

Esto puede revisarse según los requisitos de producción.

---

# 31. Diferencia entre telemetry y state

```text
telemetry
→ muestra periódica

state/reported
→ estado actual consolidado
```

No deben utilizarse indistintamente.

---

# 32. `state/reported`

Topic:

```text
maim/v1/devices/{device_id}/state/reported
```

Dirección:

```text
DISPOSITIVO → SERVIDOR
```

Propósito:

Representar el estado actual del dispositivo.

---

# 33. Estructura general de `state/reported`

```json
{
  "schema": 1,
  "ts": 1786497569,
  "device": {},
  "state": {},
  "metrics": {},
  "sensors": {},
  "outputs": {},
  "network": {},
  "firmware": {},
  "errors": []
}
```

No todas las secciones necesitan contener datos en todos los modelos.

---

# 34. `device`

Ejemplo:

```json
"device": {
  "device_id": "MM_TEST_001",
  "model": "MAIM_MINI",
  "hw_rev": "1.0"
}
```

Campos actuales:

```text
device_id
model
hw_rev
```

---

# 35. `state`

Representa estados lógicos.

Ejemplo:

```json
"state": {
  "mode": "DISPENSING",
  "busy": true
}
```

---

# 36. Ejemplos de state futuros

```json
{
  "mode": "STANDBY",
  "busy": false,
  "door_open": false
}
```

Los campos dependen de las capacidades del modelo.

---

# 37. `metrics`

Representa acumuladores o valores calculados.

Ejemplo validado:

```json
"metrics": {
  "water_total_ml": 16000,
  "last_dispense_ml": 287,
  "dispense_count": 48
}
```

---

# 38. `sensors`

Representa mediciones físicas actuales.

Ejemplo conceptual:

```json
"sensors": {
  "cold_tank_c": 7.8,
  "hot_tank_c": 81.5
}
```

Actualmente `device_manager` ya soporta la categoría SENSOR.

La exposición completa dentro de `state/reported` puede crecer según los modelos.

---

# 39. `outputs`

Representa actuadores.

Ejemplo validado:

```json
"outputs": {
  "valve": true
}
```

Un equipo mayor podría enviar:

```json
"outputs": {
  "cold_valve": false,
  "hot_valve": true,
  "compressor": true,
  "heater": false
}
```

---

# 40. `network`

Ejemplo validado:

```json
"network": {
  "type": "wifi",
  "connected": true,
  "rssi_dbm": -52,
  "ip": "192.168.1.66"
}
```

---

# 41. IP dentro del estado

La IP se publica para diagnóstico.

No debe utilizarse como identidad permanente del dispositivo.

---

# 42. `firmware`

Ejemplo:

```json
"firmware": {
  "esp32": {
    "version": "0.1.0"
  }
}
```

En el futuro puede extenderse:

```json
"firmware": {
  "esp32": {
    "version": "1.2.0"
  },
  "controller": {
    "version": "2.4.1"
  }
}
```

---

# 43. `errors`

Representa errores activos.

Ejemplo validado:

```json
"errors": [
  {
    "code": "FLOW_SENSOR_FAIL",
    "severity": "ERROR"
  }
]
```

Sin errores:

```json
"errors": []
```

---

# 44. Severidades

El protocolo interno utiliza:

```text
INFO
WARNING
ERROR
CRITICAL
```

En MQTT puede conservarse esta nomenclatura para errores.

---

# 45. Retain de state/reported

Debe utilizar:

```text
retain = true
```

Esto permite que el backend conozca inmediatamente el último estado reportado por el equipo.

---

# 46. QoS state/reported

Recomendación:

```text
QoS 1
```

---

# 47. Actualización de state/reported

Puede publicarse:

* después de conexión;
* como respuesta a `GET_STATE`;
* después de cambios importantes;
* después de snapshots;
* cuando lo determine la estrategia futura.

No debe convertirse necesariamente en telemetría de alta frecuencia.

---

# 48. `event`

Topic:

```text
maim/v1/devices/{device_id}/event
```

Dirección:

```text
DISPOSITIVO → SERVIDOR
```

Propósito:

Informar inmediatamente que ocurrió un evento.

---

# 49. Estructura EVENT

```json
{
  "schema": 1,
  "event_id": "EVT-MM_TEST_001-000001",
  "ts": 1786504167,
  "type": "DISPENSE_STARTED",
  "severity": "info",
  "data": {}
}
```

---

# 50. `event_id`

Cada evento generado por el ESP32 contiene:

```text
event_id
```

Formato actual:

```text
EVT-{DEVICE_ID}-{SEQUENCE}
```

Ejemplo:

```text
EVT-MM_TEST_001-000004
```

---

# 51. Persistencia actual del Event ID

Actualmente la secuencia:

```text
event_sequence
```

es mantenida en RAM.

Por lo tanto puede reiniciarse cuando el ESP32 reinicia.

Esto es aceptable para v0.1.0 de laboratorio.

---

# 52. Event ID futuro

En producción puede evolucionar hacia:

* secuencia persistente;
* boot ID;
* timestamp;
* UUID;
* combinación de campos.

Estado:

```text
[PLANEADO]
```

---

# 53. `type`

Identifica el tipo de evento.

Ejemplos actuales:

```text
DISPENSE_STARTED
DISPENSE_COMPLETED
DISPENSE_CANCELLED
CALIBRATION_COMPLETED
PROGRAMMING_CHANGED
```

---

# 54. `severity` de eventos

Los eventos normales actuales utilizan:

```json
"severity": "info"
```

Errores y alarmas pueden utilizar otro manejo.

---

# 55. `data`

Contiene parámetros específicos del evento.

Ejemplo:

```json
"data": {
  "volume_ml": 287,
  "water_total_ml": 16287
}
```

---

# 56. `DISPENSE_STARTED`

UART:

```text
<EVENT,DISPENSE_STARTED>
```

MQTT:

```json
{
  "schema": 1,
  "event_id": "EVT-MM_TEST_001-000001",
  "ts": 1786504167,
  "type": "DISPENSE_STARTED",
  "severity": "info",
  "data": {}
}
```

---

# 57. `DISPENSE_COMPLETED`

UART:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

Interpretación:

```text
287
→ volume_ml

16287
→ water_total_ml
```

MQTT:

```json
{
  "type": "DISPENSE_COMPLETED",
  "data": {
    "volume_ml": 287,
    "water_total_ml": 16287
  }
}
```

---

# 58. `DISPENSE_CANCELLED`

Formato previsto:

```text
<EVENT,DISPENSE_CANCELLED,124>
```

MQTT:

```json
{
  "type": "DISPENSE_CANCELLED",
  "data": {
    "volume_ml": 124
  }
}
```

---

# 59. `CALIBRATION_COMPLETED`

UART:

```text
<EVENT,CALIBRATION_COMPLETED,468.2>
```

MQTT:

```json
{
  "type": "CALIBRATION_COMPLETED",
  "data": {
    "pulses_per_liter": 468.2
  }
}
```

---

# 60. `PROGRAMMING_CHANGED`

UART:

```text
<EVENT,PROGRAMMING_CHANGED,8000>
```

MQTT:

```json
{
  "type": "PROGRAMMING_CHANGED",
  "data": {
    "dispense_time_ms": 8000
  }
}
```

---

# 61. Eventos genéricos

Para eventos todavía no especializados, los parámetros pueden representarse temporalmente como:

```json
"data": {
  "param_1": "VALUE",
  "param_2": "VALUE"
}
```

Esto permite extender el sistema sin romper el parser.

Sin embargo, los eventos estables deben definir nombres de parámetros explícitos.

---

# 62. Retain de events

Debe utilizar:

```text
retain = false
```

Un evento describe algo que ocurrió, no el estado actual.

---

# 63. QoS event

Recomendación:

```text
QoS 1
```

para eventos relevantes.

---

# 64. `command/request`

Topic:

```text
maim/v1/devices/{device_id}/command/request
```

Dirección:

```text
SERVIDOR → DISPOSITIVO
```

Propósito:

Solicitar una acción al ESP32 o al controlador principal.

---

# 65. Estructura de command/request

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "command": "DISPENSE",
  "params": {}
}
```

---

# 66. `command_id`

Identifica de forma lógica una solicitud MQTT.

Ejemplo:

```text
CMD-000200
```

Debe ser único al menos dentro del contexto necesario para relacionar una solicitud con sus respuestas.

---

# 67. `command`

Identifica la operación solicitada.

Ejemplos actuales:

```text
PING
GET_STATE
DISPENSE
STOP
```

---

# 68. `params`

Contiene parámetros específicos del comando.

Sin parámetros:

```json
"params": {}
```

Ejemplo futuro:

```json
"params": {
  "duration_ms": 5000
}
```

---

# 69. Comandos locales del ESP32

Algunos comandos pueden resolverse directamente en el ESP32.

Actualmente:

```text
PING
GET_STATE
```

No necesitan transacción UART.

---

# 70. `PING`

Request:

```json
{
  "schema": 1,
  "command_id": "CMD-000001",
  "command": "PING",
  "params": {}
}
```

Respuesta:

```json
{
  "schema": 1,
  "command_id": "CMD-000001",
  "ts": 1786504167,
  "status": "SUCCESS",
  "result": {
    "response": "PONG"
  }
}
```

---

# 71. `GET_STATE`

Request:

```json
{
  "schema": 1,
  "command_id": "CMD-000101",
  "command": "GET_STATE",
  "params": {}
}
```

El ESP32 publica primero:

```text
state/reported
```

y posteriormente:

```text
command/response
```

---

# 72. GET_STATE response

```json
{
  "schema": 1,
  "command_id": "CMD-000101",
  "ts": 1786497569,
  "status": "SUCCESS",
  "result": {
    "response": "STATE_PUBLISHED"
  }
}
```

---

# 73. Comandos dirigidos al controlador

Ejemplos:

```text
DISPENSE
STOP
```

Estos comandos generan una transacción UART.

---

# 74. Ejemplo DISPENSE

MQTT:

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "command": "DISPENSE",
  "params": {}
}
```

ESP32 genera:

```text
<CMD,1,DISPENSE>
```

---

# 75. IDs MQTT vs UART

Los IDs son diferentes:

```text
MQTT command_id:
CMD-000200

UART transaction_id:
1
```

`transaction_manager` mantiene la relación.

---

# 76. `command/response`

Topic:

```text
maim/v1/devices/{device_id}/command/response
```

Dirección:

```text
DISPOSITIVO → SERVIDOR
```

Propósito:

Informar el progreso o resultado de una solicitud.

---

# 77. Estados oficiales actuales de respuesta

```text
RECEIVED
SUCCESS
REJECTED
FAILED
```

---

# 78. `RECEIVED`

Significa:

> El controlador recibió y aceptó inicialmente el comando.

Normalmente corresponde a:

```text
<ACK,ID>
```

Ejemplo:

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "ts": 1786504167,
  "status": "RECEIVED"
}
```

---

# 79. RECEIVED no significa SUCCESS

Es importante distinguir:

```text
RECEIVED
→ operación aceptada

SUCCESS
→ operación terminada
```

Un comando puede ser aceptado pero fallar posteriormente.

---

# 80. `SUCCESS`

Normalmente corresponde a:

```text
<DONE,ID>
```

Ejemplo:

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "ts": 1786504170,
  "status": "SUCCESS"
}
```

---

# 81. `REJECTED`

Corresponde a:

```text
<NACK,ID,REASON>
```

Ejemplo UART:

```text
<NACK,2,BUSY>
```

MQTT:

```json
{
  "schema": 1,
  "command_id": "CMD-000201",
  "ts": 1786504170,
  "status": "REJECTED",
  "error": {
    "code": "BUSY"
  }
}
```

---

# 82. Posibles razones NACK

Inicialmente:

```text
UNKNOWN_COMMAND
INVALID_PARAM
OUT_OF_RANGE
BUSY
NOT_ALLOWED
SAFETY_LOCK
NO_WATER
LOW_BATTERY
INTERNAL_ERROR
```

La lista puede crecer manteniendo la misma estructura.

---

# 83. `FAILED`

Representa un fallo de ejecución o comunicación después de haber iniciado el proceso.

Ejemplos actuales:

```text
ACK_TIMEOUT
EXECUTION_TIMEOUT
```

---

# 84. ACK Timeout MQTT

Ejemplo:

```json
{
  "schema": 1,
  "command_id": "CMD-TIMEOUT-001",
  "ts": 1786504170,
  "status": "FAILED",
  "error": {
    "code": "ACK_TIMEOUT"
  }
}
```

---

# 85. Execution Timeout MQTT

```json
{
  "schema": 1,
  "command_id": "CMD-TIMEOUT-002",
  "ts": 1786504200,
  "status": "FAILED",
  "error": {
    "code": "EXECUTION_TIMEOUT"
  }
}
```

---

# 86. Diferencia REJECTED vs FAILED

```text
REJECTED
→ controlador decidió no ejecutar

FAILED
→ la transacción no pudo completarse correctamente
```

Ejemplo:

```text
BUSY
→ REJECTED

ACK_TIMEOUT
→ FAILED
```

---

# 87. Retain de command/request

Debe ser:

```text
false
```

Esta es una regla crítica.

Nunca deben utilizarse comandos retained.

---

# 88. Riesgo de comandos retained

Si un comando como:

```text
DISPENSE
```

fuera retained, un dispositivo que se reconectara podría recibir nuevamente una orden antigua.

Por seguridad:

```text
command/request → retain false
```

siempre.

---

# 89. Retain de command/response

Debe utilizar:

```text
false
```

Las respuestas pertenecen a transacciones específicas y no deben ejecutarse o interpretarse como estado persistente.

---

# 90. QoS command/request

Recomendación:

```text
QoS 1
```

---

# 91. QoS command/response

Recomendación:

```text
QoS 1
```

---

# 92. Duplicados con QoS 1

MQTT QoS 1 significa:

```text
at least once
```

Por lo tanto un mensaje puede entregarse más de una vez.

La arquitectura futura debe considerar deduplicación mediante:

```text
command_id
```

Actualmente la gestión completa de duplicados aún no está congelada.

Estado:

```text
[PLANEADO]
```

---

# 93. Regla futura de idempotencia

Los comandos sensibles deben poder detectar un `command_id` ya procesado.

Ejemplo:

```text
CMD-000200
```

no debería provocar dos dispensaciones si se recibe duplicado debido a retransmisión MQTT.

Esto será especialmente importante en producción.

---

# 94. Errores de formato

Un mensaje inválido debe rechazarse sin provocar acciones físicas.

Ejemplos:

```json
{}
```

```json
{
  "command": "DISPENSE"
}
```

sin `command_id` o `schema` cuando sean obligatorios.

---

# 95. Validación antes de actuar

Flujo recomendado:

```text
recibir JSON
   ↓
validar JSON
   ↓
validar schema
   ↓
validar command_id
   ↓
validar command
   ↓
validar params
   ↓
ejecutar / transmitir UART
```

Nunca ejecutar primero y validar después.

---

# 96. Comandos desconocidos

Un comando que el ESP32 no reconozca debe generar una respuesta controlada.

Ejemplo conceptual:

```json
{
  "status": "REJECTED",
  "error": {
    "code": "UNKNOWN_COMMAND"
  }
}
```

---

# 97. Campos obligatorios de command/request

Para v1:

```text
schema
command_id
command
params
```

`params` puede estar vacío:

```json
{}
```

pero debe mantener una estructura consistente.

---

# 98. Campos obligatorios de command/response

Como mínimo:

```text
schema
command_id
ts
status
```

Dependiendo del resultado puede incluir:

```text
result
```

o:

```text
error
```

---

# 99. `result`

Se utiliza para datos de resultado positivo.

Ejemplo:

```json
"result": {
  "response": "PONG"
}
```

---

# 100. `error`

Se utiliza para resultados negativos.

Ejemplo:

```json
"error": {
  "code": "BUSY"
}
```

Puede extenderse en el futuro con:

```json
{
  "code": "BUSY",
  "detail": "..."
}
```

si fuera necesario.

---

# 101. Separación entre error activo y error de comando

No deben confundirse:

```text
state/reported → errors[]
```

con:

```text
command/response → error
```

`errors[]` representa fallas activas del equipo.

`command/response.error` explica por qué falló una transacción específica.

---

# 102. Ejemplo

Equipo tiene:

```text
FLOW_SENSOR_FAIL
```

activo.

Entonces `state/reported` puede contener:

```json
"errors": [
  {
    "code": "FLOW_SENSOR_FAIL",
    "severity": "ERROR"
  }
]
```

Si además rechaza DISPENSE:

```json
{
  "status": "REJECTED",
  "error": {
    "code": "FLOW_SENSOR_FAIL"
  }
}
```

Ambos mensajes tienen propósitos distintos.

---

# 103. Estado vs evento

Otra distinción importante:

```text
state
→ condición actual

event
→ transición o hecho ocurrido
```

Ejemplo:

```text
state:
mode = STANDBY
```

Evento:

```text
DISPENSE_COMPLETED
```

---

# 104. Evento no sustituye estado

Después de un evento:

```text
DISPENSE_COMPLETED
```

el backend puede querer conocer también:

```text
mode
busy
outputs
errors
```

Esto pertenece a `state/reported`.

---

# 105. Telemetry no sustituye event

Un evento importante no debe esperar el ciclo de 30 segundos de telemetría.

Debe publicarse inmediatamente.

---

# 106. Topic wildcard de dispositivo

Para escuchar todo un equipo:

```text
maim/v1/devices/{device_id}/#
```

Ejemplo:

```text
maim/v1/devices/MM_TEST_001/#
```

---

# 107. Todos los dispositivos

Para escuchar availability:

```text
maim/v1/devices/+/availability
```

---

# 108. Todos los eventos

Conceptualmente:

```text
maim/v1/devices/+/event
```

Esto permitirá a un backend consumir eventos de toda la flota.

---

# 109. Dirección de topics

| Topic              | Dirección       |
| ------------------ | --------------- |
| `availability`     | Device → Server |
| `telemetry`        | Device → Server |
| `state/reported`   | Device → Server |
| `event`            | Device → Server |
| `command/request`  | Server → Device |
| `command/response` | Device → Server |

---

# 110. Resumen QoS / Retain

| Topic              | QoS recomendado | Retain |
| ------------------ | --------------: | ------ |
| `availability`     |               1 | Sí     |
| `telemetry`        |               0 | No     |
| `state/reported`   |               1 | Sí     |
| `event`            |               1 | No     |
| `command/request`  |               1 | No     |
| `command/response` |               1 | No     |

---

# 111. Flujo de conexión

Cuando el ESP32 inicia:

```text
Wi-Fi
 ↓
SNTP
 ↓
MQTT connect
 ↓
Subscribe command/request
 ↓
Publish availability online
 ↓
Publish state/reported
 ↓
telemetry periódica
```

---

# 112. Suscripción del dispositivo

Actualmente el ESP32 se suscribe a:

```text
maim/v1/devices/{device_id}/command/request
```

Esto evita escuchar comandos de otros dispositivos.

---

# 113. Topics construidos dinámicamente

El firmware construye topics utilizando:

```text
MAIM_MQTT_ROOT
+
MAIM_DEVICE_ID
```

Ejemplo:

```text
MAIM_MQTT_ROOT
= maim/v1/devices

MAIM_DEVICE_ID
= MM_TEST_001
```

Resultado:

```text
maim/v1/devices/MM_TEST_001
```

---

# 114. Broker actual

Durante laboratorio:

```text
mqtt://192.168.1.85:1883
```

Este valor no forma parte del protocolo.

Es únicamente configuración de infraestructura.

---

# 115. Seguridad actual

El laboratorio utiliza:

```text
username
password
TCP 1883
```

No utiliza todavía:

```text
TLS
```

Por lo tanto:

```text
[LABORATORIO]
```

---

# 116. Producción futura

Debe evaluarse:

```text
MQTTS
TLS
puerto 8883
certificados
ACL
credenciales por dispositivo
revocación
rotación
```

---

# 117. ACL futura

Idealmente un dispositivo:

```text
MM_00001234
```

solo debería tener acceso a:

```text
maim/v1/devices/MM_00001234/#
```

y no a topics de otros equipos.

---

# 118. Credenciales por equipo

No utilizar una única cuenta compartida para toda la flota en producción.

Estado:

```text
[PLANEADO]
```

---

# 119. Compatibilidad futura

Agregar un nuevo campo opcional a:

```text
state/reported
```

no debe romper consumidores v1 que ignoren campos desconocidos.

---

# 120. Regla para consumidores

Los consumidores deben:

```text
procesar campos conocidos
ignorar campos opcionales desconocidos
```

siempre que `schema` sea compatible.

---

# 121. Cambio incompatible

Un cambio que altere significativamente:

* significado de un campo;
* formato fundamental;
* estructura de comandos;
* semántica de respuestas;

puede requerir una nueva versión.

Ejemplo:

```text
v2
```

---

# 122. No reutilizar nombres con significado distinto

Si:

```text
water_total_ml
```

significa volumen acumulado en mililitros, nunca debe reutilizarse posteriormente para litros.

Debe crearse otro campo o nueva versión.

---

# 123. Tipado estable

Una vez definido:

```json
"busy": true
```

no debe convertirse en una versión posterior del mismo schema a:

```json
"busy": "1"
```

Mantener tipos estables.

---

# 124. Campos opcionales

Un modelo que no tenga determinado sensor puede simplemente omitirlo.

Ejemplo:

MAIM MINI:

```json
"sensors": {}
```

Otro modelo:

```json
"sensors": {
  "cold_tank_c": 7.8
}
```

---

# 125. No usar `null` innecesariamente

Preferencia actual:

```text
omitir campo desconocido
```

en lugar de:

```json
"cold_tank_c": null
```

salvo que la diferencia entre "desconocido" y "no existente" sea necesaria.

---

# 126. Estado parcial

El sistema actual puede construir únicamente campos conocidos por `device_manager`.

Esto permite que `state/reported` crezca gradualmente.

---

# 127. Snapshot futuro

El protocolo UART permite:

```text
<SNAPSHOT,BEGIN,ID>
...
<SNAPSHOT,END,ID>
```

Esto permitirá construir un `state/reported` coherente después de sincronización con el controlador.

---

# 128. Inicio después de reinicio del ESP32

Una evolución prevista es:

```text
ESP32 reinicia
   ↓
solicita GET_STATE al controlador
   ↓
SNAPSHOT
   ↓
reconstruye Device Manager
   ↓
publica state/reported
```

Esto garantizará que el backend reciba un estado actualizado después del reinicio.

Estado:

```text
[PLANEADO]
```

---

# 129. Uso de JSON

El firmware utiliza:

```text
cJSON
```

para construir y analizar payloads.

Debe evitarse construir JSON complejo manualmente mediante concatenación de strings cuando exista una alternativa segura.

---

# 130. Memoria

Los objetos cJSON deben eliminarse después de publicar o procesar.

Ejemplo:

```c
cJSON_Delete(root);
```

para evitar fugas de memoria.

---

# 131. Precisión numérica

Valores decimales deben manejarse evitando artefactos innecesarios.

Durante eventos apareció inicialmente:

```json
"pulses_per_liter": 468.20001220703125
```

por conversión desde `float`.

Se prefirió utilizar:

```c
strtod()
```

para obtener una representación JSON más limpia:

```json
"pulses_per_liter": 468.2
```

---

# 132. No enviar secretos

Nunca deben publicarse por MQTT:

* contraseña Wi-Fi;
* contraseña MQTT;
* claves privadas;
* tokens;
* secretos internos.

---

# 133. Información sensible futura

Si el backend necesita realizar provisioning o rotación de credenciales deberá utilizar mecanismos específicos y protegidos.

No reutilizar `telemetry` o `event` para transportar secretos.

---

# 134. Comandos físicos

Los comandos remotos no sustituyen las protecciones del controlador.

Ejemplo:

```text
MQTT → DISPENSE
```

no significa:

```text
abrir válvula directamente desde ESP32
```

La ruta correcta es:

```text
MQTT
 ↓
ESP32
 ↓
UART CMD
 ↓
controlador valida
 ↓
actuador
```

---

# 135. Autoridad del controlador

El controlador puede responder:

```text
NACK
```

si:

* está ocupado;
* falta agua;
* existe una falla;
* existe bloqueo de seguridad;
* los parámetros son inválidos.

---

# 136. Timeout MQTT/UART

Si el controlador no responde:

```text
ACK_TIMEOUT
```

Si acepta pero no termina:

```text
EXECUTION_TIMEOUT
```

Estos errores se reportan mediante `command/response`.

---

# 137. Command ID y correlación

El backend debe conservar:

```text
command_id
```

hasta recibir resultado final.

Ejemplo:

```text
CMD-000200
```

puede producir:

```text
RECEIVED
```

y luego:

```text
SUCCESS
```

con el mismo ID.

---

# 138. Una transacción puede producir varias respuestas

Ejemplo:

```text
command/request CMD-000200
       ↓
response RECEIVED
       ↓
response SUCCESS
```

Por tanto, el backend no debe asumir que la primera respuesta siempre es terminal.

---

# 139. Estados terminales

Actualmente son terminales:

```text
SUCCESS
REJECTED
FAILED
```

No terminal:

```text
RECEIVED
```

---

# 140. Backend futuro

El backend puede modelar:

```text
PENDING
  ↓
RECEIVED
  ↓
SUCCESS
```

o:

```text
PENDING
  ↓
REJECTED
```

o:

```text
PENDING
  ↓
FAILED
```

---

# 141. Posible estado RUNNING futuro

Si existen operaciones largas podría introducirse:

```text
RUNNING
```

pero todavía no forma parte de v1.

No utilizarlo hasta documentarlo formalmente.

---

# 142. Comandos actualmente validados

```text
PING
GET_STATE
DISPENSE
STOP
```

`DISPENSE` y `STOP` están preparados como comandos dirigidos al controlador.

---

# 143. Eventos actualmente validados

```text
DISPENSE_STARTED
DISPENSE_COMPLETED
CALIBRATION_COMPLETED
PROGRAMMING_CHANGED
```

También se preparó soporte para:

```text
DISPENSE_CANCELLED
```

---

# 144. Errores actualmente validados

Ejemplo:

```text
FLOW_SENSOR_FAIL
```

con:

```text
severity = ERROR
```

La estructura permite agregar otros errores sin modificar el protocolo base.

---

# 145. Escuchar todos los mensajes de un dispositivo

Rocky Linux:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/<DEVICE_ID>/#" \
-v
```

---

# 146. GET_STATE desde PowerShell

```powershell
.\mosquitto_pub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/<DEVICE_ID>/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-000101\",\"command\":\"GET_STATE\",\"params\":{}}'
```

---

# 147. DISPENSE desde PowerShell

```powershell
.\mosquitto_pub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/<DEVICE_ID>/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-000200\",\"command\":\"DISPENSE\",\"params\":{}}'
```

---

# 148. Flujo completo dispositivo → servidor

```text
ATmega
  │
  │ UART
  ▼
ESP32
  │
  ├── state
  ├── metrics
  ├── events
  └── errors
  │
  ▼
MQTT
  │
  ▼
Mosquitto
  │
  ▼
Backend
```

---

# 149. Flujo completo servidor → dispositivo

```text
Backend
  │
  │ command/request
  ▼
Mosquitto
  │
  ▼
ESP32
  │
  │ CMD UART
  ▼
ATmega
  │
  ├── ACK
  ├── DONE
  └── NACK
  │
  ▼
ESP32
  │
  ▼
command/response
```

---

# 150. Principios del protocolo

MAIM MQTT v1 debe mantenerse:

```text
simple
legible
versionable
extensible
diagnosticable
independiente del modelo
seguro ante comandos repetidos
separado del control físico
```

---

# 151. Reglas principales

1. Cada equipo utiliza un `device_id`.
2. Todos los topics MAIM v1 comienzan con `maim/v1/devices/`.
3. Los payloads utilizan JSON.
4. Todo mensaje temporal debe incluir `ts` cuando sea posible.
5. Los timestamps utilizan Unix UTC.
6. Los comandos tienen `command_id`.
7. Los eventos tienen `event_id`.
8. Los comandos nunca son retained.
9. Los eventos nunca son retained.
10. `state/reported` sí puede ser retained.
11. `availability` sí debe ser retained.
12. El controlador mantiene autoridad sobre acciones físicas.
13. Las respuestas de transacción pueden tener múltiples etapas.
14. Los errores de equipo y errores de comando son conceptos distintos.
15. Los cambios incompatibles requieren versionado.

---

# 152. Estado actual

```text
[PROBADO] raíz maim/v1/devices
[PROBADO] availability
[PROBADO] Last Will
[PROBADO] telemetry
[PROBADO] state/reported
[PROBADO] event
[PROBADO] command/request
[PROBADO] command/response
[PROBADO] schema
[PROBADO] Unix timestamp
[PROBADO] PING
[PROBADO] GET_STATE
[PROBADO] DISPENSE
[PROBADO] ACK → RECEIVED
[PROBADO] DONE → SUCCESS
[PROBADO] NACK → REJECTED
[PROBADO] ACK_TIMEOUT → FAILED
[PROBADO] EXECUTION_TIMEOUT → FAILED

[PLANEADO] deduplicación por command_id
[PLANEADO] TLS
[PLANEADO] ACL
[PLANEADO] credenciales por dispositivo
[PLANEADO] config/desired
[PLANEADO] config/reported
[PLANEADO] firmware/desired
[PLANEADO] firmware/reported
```

---

# 153. Expansión futura de topics

La estructura está preparada para crecer con ramas como:

```text
maim/v1/devices/{device_id}/config/desired
maim/v1/devices/{device_id}/config/reported
```

y:

```text
maim/v1/devices/{device_id}/firmware/desired
maim/v1/devices/{device_id}/firmware/reported
```

Estas ramas todavía no forman parte de la implementación funcional actual.

---

# 154. Relación con otros documentos

Mosquitto:

```text
03_MQTT_MOSQUITTO.md
```

Sincronización de hora:

```text
07_SINCRONIZACION_HORA.md
```

Protocolo UART:

```text
09_PROTOCOLO_UART_INTERNO.md
```

Device Manager:

```text
10_DEVICE_MANAGER.md
```

Eventos:

```text
11_EVENTOS.md
```

Comandos:

```text
12_COMANDOS_Y_TRANSACCIONES.md
```

Timeouts:

```text
13_TIMEOUTS.md
```

Pruebas:

```text
14_PRUEBAS_Y_DIAGNOSTICO.md
```

---

# 155. Resumen

MAIM MQTT Protocol v1 establece una arquitectura común basada en:

```text
availability
telemetry
state/reported
event
command/request
command/response
```

con:

```text
Device ID
Schema
Unix timestamps
JSON
ACK / DONE / NACK
timeouts
```

La intención es que la infraestructura pueda crecer desde equipos simples hasta equipos con múltiples sensores, actuadores y funciones sin abandonar el mismo modelo de comunicaciones.

Este documento constituye la **especificación MQTT base de MAIM Connectivity v0.1.0**.
