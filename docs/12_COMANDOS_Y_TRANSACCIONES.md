# MAIM Connectivity

## Comandos y Transacciones

**Documento:** `12_COMANDOS_Y_TRANSACCIONES.md`
**Proyecto:** MAIM Connectivity
**Módulos principales:** `mqtt_manager`, `transaction_manager`, `uart_protocol`, `device_manager`
**Protocolo MQTT:** MAIM MQTT Protocol v1
**Protocolo UART:** MAIM Internal UART Protocol v1
**Estado:** [PROBADO] Ruta servidor → controlador → servidor funcional

---

# 1. Propósito

Este documento describe el sistema utilizado para ejecutar acciones remotas sobre un equipo MAIM y conocer de forma confiable su resultado.

La arquitectura utiliza una cadena de comunicación:

```text
Servidor
   ↓
MQTT command/request
   ↓
ESP32
   ↓
transaction_manager
   ↓
CMD UART
   ↓
Controlador principal
   ↓
ACK / DONE / NACK
   ↓
ESP32
   ↓
MQTT command/response
   ↓
Servidor
```

El objetivo principal es evitar que una orden remota sea tratada simplemente como:

```text
"mensaje enviado"
```

La infraestructura debe poder distinguir si el controlador:

```text
recibió
aceptó
rechazó
terminó
o dejó de responder
```

---

# 2. Principio general

Una solicitud remota se considera una **transacción**.

Cada transacción tiene:

```text
command_id MQTT
+
transaction_id UART
+
estado interno
+
resultado
```

Esto permite correlacionar una solicitud realizada por el servidor con las respuestas recibidas posteriormente desde el controlador.

---

# 3. Responsabilidades por capa

La arquitectura se divide de la siguiente manera.

## `mqtt_manager`

Responsable de:

```text
recibir command/request
validar JSON
identificar comando
publicar command/response
```

## `transaction_manager`

Responsable de:

```text
crear transacción
asignar ID UART
relacionar MQTT ID ↔ UART ID
mantener estado
procesar ACK
procesar DONE
procesar NACK
supervisar timeouts
liberar transacciones
```

## `uart_protocol`

Responsable de:

```text
construir CMD
parsear ACK
parsear DONE
parsear NACK
```

## `device_manager`

Responsable de:

```text
validar respuestas UART
extraer transaction_id
enviar respuesta a transaction_manager
```

---

# 4. Topic MQTT de entrada

Los comandos remotos llegan a:

```text
maim/v1/devices/{device_id}/command/request
```

Ejemplo:

```text
maim/v1/devices/MM_TEST_001/command/request
```

Dirección:

```text
SERVIDOR → DISPOSITIVO
```

---

# 5. Topic MQTT de respuesta

Las respuestas se publican en:

```text
maim/v1/devices/{device_id}/command/response
```

Dirección:

```text
DISPOSITIVO → SERVIDOR
```

---

# 6. Estructura `command/request`

Formato general:

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "command": "DISPENSE",
  "params": {}
}
```

---

# 7. Campo `schema`

Actualmente:

```json
"schema": 1
```

identifica la versión del payload.

Debe validarse antes de ejecutar la operación.

---

# 8. Campo `command_id`

Ejemplo:

```text
CMD-000200
```

Es el identificador de la solicitud MQTT.

Permite correlacionar:

```text
request
↓
RECEIVED
↓
SUCCESS
```

o:

```text
request
↓
REJECTED
```

o:

```text
request
↓
FAILED
```

---

# 9. Importancia del `command_id`

Sin `command_id`, si existen varios comandos simultáneos sería difícil saber a cuál pertenece una respuesta.

Ejemplo:

```text
DISPENSE
STOP
GET_STATE
```

pueden encontrarse en distintas etapas.

El ID permite relacionarlos correctamente.

---

# 10. Campo `command`

Ejemplos actualmente utilizados:

```text
PING
GET_STATE
DISPENSE
STOP
```

No todos los comandos siguen la misma ruta.

---

# 11. Campo `params`

Contiene parámetros específicos.

Sin parámetros:

```json
"params": {}
```

Ejemplo futuro:

```json
"params": {
  "volume_ml": 350
}
```

La validación concreta depende del comando.

---

# 12. Tipos de comando

Actualmente existen dos grandes categorías:

```text
COMANDOS LOCALES ESP32
```

y:

```text
COMANDOS DIRIGIDOS AL CONTROLADOR
```

---

# 13. Comandos locales ESP32

Son operaciones que el ESP32 puede resolver directamente.

Actualmente:

```text
PING
GET_STATE
```

No necesitan crear una transacción UART.

---

# 14. PING

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

# 15. GET_STATE

Request:

```json
{
  "schema": 1,
  "command_id": "CMD-000101",
  "command": "GET_STATE",
  "params": {}
}
```

El ESP32 publica:

```text
state/reported
```

y después:

```text
command/response
```

con:

```text
SUCCESS
STATE_PUBLISHED
```

---

# 16. Resultado GET_STATE validado

Durante las pruebas se observó:

```text
command/request
```

seguido de:

```text
state/reported
```

y después:

```text
command/response
```

Estado:

```text
[PROBADO]
```

---

# 17. Comandos dirigidos al controlador

Los comandos físicos pasan por UART.

Ejemplos:

```text
DISPENSE
STOP
```

Ruta:

```text
MQTT
 ↓
transaction_manager
 ↓
UART CMD
 ↓
ATmega
```

---

# 18. Creación de una transacción

Cuando llega:

```text
command_id = CMD-000200
command = DISPENSE
```

`transaction_manager` crea una entrada interna.

Conceptualmente:

```text
MQTT ID:
CMD-000200

UART ID:
1

COMMAND:
DISPENSE

STATE:
WAITING_ACK
```

---

# 19. IDs diferentes

El protocolo MQTT utiliza strings:

```text
CMD-000200
```

mientras UART utiliza un entero pequeño:

```text
1
```

Esto reduce la cantidad de bytes enviados por UART.

---

# 20. Relación interna

`transaction_manager` mantiene:

```text
CMD-000200 ↔ 1
```

Así, cuando llega:

```text
<ACK,1>
```

el ESP32 sabe que pertenece a:

```text
CMD-000200
```

---

# 21. Estructura interna

Conceptualmente:

```c
typedef struct
{
    bool used;

    uint16_t uart_id;

    char mqtt_command_id[];
    char command[];

    transaction_state_t state;

    int64_t created_us;
    int64_t ack_received_us;

} transaction_entry_t;
```

---

# 22. Estados internos

Actualmente:

```text
FREE
WAITING_ACK
ACKED
COMPLETED
REJECTED
FAILED
```

---

# 23. `FREE`

El slot no contiene una transacción activa.

---

# 24. `WAITING_ACK`

El comando ya fue enviado por UART y se espera:

```text
<ACK,ID>
```

o:

```text
<NACK,ID,REASON>
```

---

# 25. `ACKED`

El controlador confirmó que recibió y aceptó la operación.

Se espera:

```text
<DONE,ID>
```

---

# 26. `COMPLETED`

La operación terminó correctamente.

Corresponde a:

```text
<DONE,ID>
```

---

# 27. `REJECTED`

El controlador rechazó la operación.

Corresponde a:

```text
<NACK,ID,REASON>
```

---

# 28. `FAILED`

La transacción no pudo completarse.

Ejemplos actuales:

```text
ACK_TIMEOUT
EXECUTION_TIMEOUT
```

---

# 29. Asignación de UART ID

Los IDs UART utilizan:

```text
uint16_t
```

Rango:

```text
1 ... 65535
```

El valor:

```text
0
```

se reserva y no se utiliza normalmente.

---

# 30. Secuencia de IDs

Ejemplo:

```text
1
2
3
4
...
65535
```

Después puede volver a:

```text
1
```

evitando colisiones con transacciones activas.

---

# 31. Número máximo de transacciones activas

Actualmente se ha definido una tabla limitada.

Ejemplo:

```text
TRANSACTION_MANAGER_MAX_ACTIVE = 8
```

Esto evita asignación dinámica ilimitada.

---

# 32. Por qué limitar transacciones

Un equipo físico no necesita mantener cientos de operaciones simultáneas.

Limitar slots permite:

```text
uso de RAM predecible
protección ante abuso
lógica simple
```

---

# 33. Generación de CMD UART

Una vez creada la transacción:

```text
transaction_manager
```

utiliza:

```text
uart_protocol
```

para construir:

```text
<CMD,1,DISPENSE>
```

---

# 34. Formato CMD

```text
<CMD,TRANSACTION_ID,COMMAND[,PARAMS...]>
```

Ejemplo:

```text
<CMD,1,DISPENSE>
```

---

# 35. Estado después de transmitir

Una vez enviada la trama:

```text
state = WAITING_ACK
```

y se registra:

```text
created_us
```

para supervisar timeout.

---

# 36. ACK

El controlador responde:

```text
<ACK,1>
```

Significa:

> El controlador recibió la solicitud y la aceptó inicialmente.

---

# 37. Ruta ACK

```text
<ACK,1>
   ↓
uart_protocol
   ↓
device_manager
   ↓
transaction_manager
   ↓
buscar UART ID = 1
   ↓
CMD-000200
```

---

# 38. Cambio de estado al recibir ACK

```text
WAITING_ACK
      ↓
ACK
      ↓
ACKED
```

También se registra:

```text
ack_received_us
```

---

# 39. MQTT después de ACK

Se publica:

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "ts": 1786504167,
  "status": "RECEIVED"
}
```

---

# 40. Significado de RECEIVED

`RECEIVED` significa:

> El controlador recibió y aceptó el comando.

No significa:

> La operación terminó.

---

# 41. Diferencia crítica

```text
ACK
→ RECEIVED
```

mientras:

```text
DONE
→ SUCCESS
```

---

# 42. DONE

Cuando la operación termina:

```text
<DONE,1>
```

---

# 43. Ruta DONE

```text
<DONE,1>
   ↓
device_manager
   ↓
transaction_manager
   ↓
buscar ID 1
   ↓
CMD-000200
```

---

# 44. Estado después de DONE

```text
ACKED
  ↓
DONE
  ↓
COMPLETED
```

---

# 45. MQTT después de DONE

Se publica:

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "ts": 1786504170,
  "status": "SUCCESS"
}
```

---

# 46. Liberación de transacción

Después de:

```text
SUCCESS
```

el slot se libera.

Conceptualmente:

```text
used = false
```

y puede utilizarse para una nueva operación.

---

# 47. Importancia de liberar el slot

Una transacción terminada no debe permanecer activa.

De lo contrario:

```text
tabla llena
↓
nuevos comandos rechazados
```

---

# 48. NACK

El controlador puede rechazar una solicitud.

Formato:

```text
<NACK,ID,REASON>
```

Ejemplo:

```text
<NACK,2,BUSY>
```

---

# 49. Significado de NACK

El controlador está indicando:

> Comprendí la solicitud, pero no la ejecutaré.

---

# 50. MQTT NACK

Ejemplo:

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

# 51. Razones NACK

Inicialmente se contemplan:

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

Cada nuevo motivo debe documentarse.

---

# 52. REJECTED vs FAILED

Esta diferencia es importante.

```text
REJECTED
```

significa:

> El controlador decidió explícitamente no ejecutar.

```text
FAILED
```

significa:

> La transacción no pudo completarse correctamente.

---

# 53. Ejemplo REJECTED

```text
<NACK,15,NO_WATER>
```

Resultado:

```text
REJECTED
```

---

# 54. Ejemplo FAILED

No llega ACK.

Resultado:

```text
FAILED
ACK_TIMEOUT
```

---

# 55. Callback de resultados

`transaction_manager` informa los cambios mediante un callback.

Conceptualmente:

```c
typedef void (*transaction_manager_result_callback_t)(
    const char *mqtt_command_id,
    transaction_state_t state,
    const char *reason
);
```

---

# 56. Función del callback

Permite desacoplar:

```text
transaction_manager
```

de:

```text
mqtt_manager
```

`transaction_manager` no necesita construir JSON.

Solo informa:

```text
CMD-000200
ACKED
```

o:

```text
CMD-000200
COMPLETED
```

o:

```text
CMD-000200
FAILED
ACK_TIMEOUT
```

---

# 57. Mapping interno → MQTT

Actualmente:

| Transaction State | MQTT Status |
| ----------------- | ----------- |
| `ACKED`           | `RECEIVED`  |
| `COMPLETED`       | `SUCCESS`   |
| `REJECTED`        | `REJECTED`  |
| `FAILED`          | `FAILED`    |

---

# 58. Ejemplo completo de éxito

Servidor publica:

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "command": "DISPENSE",
  "params": {}
}
```

ESP32:

```text
<CMD,7,DISPENSE>
```

Controlador:

```text
<ACK,7>
```

MQTT:

```text
CMD-000200
RECEIVED
```

Controlador:

```text
<DONE,7>
```

MQTT:

```text
CMD-000200
SUCCESS
```

---

# 59. Flujo visual de éxito

```text
Servidor
   │
   │ CMD-000200 DISPENSE
   ▼
ESP32
   │
   │ UART ID 7
   ▼
<CMD,7,DISPENSE>
   │
   ▼
ATmega
   │
   ├── <ACK,7>
   │
   ▼
ESP32
   │
   └── RECEIVED
   │
   ▼
ATmega ejecuta
   │
   └── <DONE,7>
   ▼
ESP32
   │
   └── SUCCESS
   ▼
Servidor
```

---

# 60. Ejemplo completo de rechazo

Servidor:

```text
DISPENSE
```

ESP32:

```text
<CMD,8,DISPENSE>
```

Controlador:

```text
<NACK,8,BUSY>
```

Resultado MQTT:

```text
REJECTED
BUSY
```

---

# 61. Ejemplo de controlador sin respuesta

ESP32:

```text
<CMD,9,DISPENSE>
```

No llega:

```text
ACK
```

Después de aproximadamente:

```text
5 segundos
```

Resultado:

```text
FAILED
ACK_TIMEOUT
```

---

# 62. Ejemplo de operación congelada

ESP32:

```text
<CMD,10,DISPENSE>
```

Controlador:

```text
<ACK,10>
```

pero nunca:

```text
<DONE,10>
```

Después de aproximadamente:

```text
30 segundos
```

Resultado:

```text
FAILED
EXECUTION_TIMEOUT
```

---

# 63. Timeouts

Los mecanismos de timeout se documentan en detalle en:

```text
13_TIMEOUTS.md
```

Este documento solo describe su relación con las transacciones.

---

# 64. ACK duplicado

Puede ocurrir:

```text
<ACK,15>
<ACK,15>
```

La primera respuesta mueve:

```text
WAITING_ACK → ACKED
```

La segunda debe ignorarse o registrarse como duplicada.

No debe generar dos:

```text
RECEIVED
```

innecesariamente.

---

# 65. ACK fuera de estado

Si llega ACK cuando la transacción ya no está:

```text
WAITING_ACK
```

debe tratarse como respuesta inesperada.

---

# 66. DONE sin ACK

Conceptualmente podría recibirse:

```text
<DONE,15>
```

sin haber recibido previamente:

```text
<ACK,15>
```

La política actual debe mantener una máquina de estados coherente.

Una respuesta fuera de secuencia debe registrarse y no generar comportamientos ambiguos.

---

# 67. Respuesta después de timeout

Escenario:

```text
<CMD,15,DISPENSE>
↓
ACK_TIMEOUT
↓
slot liberado
↓
llega <ACK,15>
```

El ACK ya no pertenece a una transacción activa.

Debe registrarse como:

```text
transaction ID desconocido
```

y no reabrir la solicitud anterior.

---

# 68. Respuestas tardías

La misma regla aplica a:

```text
DONE
NACK
```

después de que una transacción terminó o expiró.

---

# 69. Transaction ID desconocido

Ejemplo:

```text
<DONE,999>
```

sin una entrada activa.

Resultado esperado:

```text
warning
transaction not found
```

No debe generar MQTT SUCCESS.

---

# 70. Comandos simultáneos

El sistema permite mantener múltiples slots activos.

Sin embargo, que existan múltiples transacciones en memoria no significa que el controlador físico deba ejecutar todas simultáneamente.

El controlador puede responder:

```text
NACK BUSY
```

---

# 71. Autoridad física

Ejemplo:

```text
CMD 1 DISPENSE
→ ACK

CMD 2 CALIBRATION
→ NACK BUSY
```

Esto es correcto.

El ESP32 administra comunicaciones.

El ATmega administra la lógica física.

---

# 72. Regla arquitectónica

> **El servidor solicita. El ESP32 transporta. El controlador decide.**

---

# 73. No controlar GPIO desde MQTT

Incorrecto:

```text
command = VALVE_ON
```

si eso significa abrir directamente un GPIO desde el ESP32 sin validación local.

Preferir una operación semántica:

```text
DISPENSE
```

que el controlador evalúa.

---

# 74. Comandos abstractos

Esto permite modificar internamente la máquina sin cambiar el protocolo externo.

Ejemplo:

```text
DISPENSE
```

podría requerir:

```text
bomba
válvula
sensor
temporizador
caudalímetro
```

pero el servidor no necesita conocer esos detalles.

---

# 75. Parámetros futuros

Un comando podrá incluir parámetros.

MQTT:

```json
{
  "command": "DISPENSE",
  "params": {
    "volume_ml": 350
  }
}
```

UART podría convertirse en:

```text
<CMD,17,DISPENSE,350>
```

La semántica debe documentarse antes de implementar.

---

# 76. Validación de parámetros

Antes de crear una transacción:

```text
validar JSON
↓
validar params
↓
validar rangos
```

Ejemplo:

```text
volume_ml = -100
```

debe rechazarse antes de intentar ejecutar.

---

# 77. Segunda validación en controlador

Aunque ESP32 valide:

```text
volume_ml
```

el controlador debe volver a validar.

Esto protege contra:

* errores de firmware;
* corrupción;
* incompatibilidad;
* condiciones físicas cambiantes.

---

# 78. Filosofía de doble validación

```text
Servidor
→ validación de negocio

ESP32
→ validación de protocolo

Controlador
→ validación física y seguridad
```

---

# 79. `command/response`

Estructura mínima:

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "ts": 1786504167,
  "status": "RECEIVED"
}
```

---

# 80. Respuesta SUCCESS con result

Puede incluir:

```json
"result": {}
```

cuando exista información útil.

Ejemplo:

```json
{
  "status": "SUCCESS",
  "result": {
    "response": "PONG"
  }
}
```

---

# 81. Respuesta negativa

Utiliza:

```json
"error": {
  "code": "BUSY"
}
```

---

# 82. `result` y `error`

No deben utilizarse simultáneamente sin una razón explícita.

Preferencia:

```text
SUCCESS
→ result
```

```text
REJECTED / FAILED
→ error
```

---

# 83. Timestamp de respuesta

Cada respuesta utiliza:

```text
ts
```

obtenido mediante:

```text
time_manager
```

Esto permite conocer cuándo el dispositivo generó la respuesta.

---

# 84. Timestamp vs timeout clock

El timestamp MQTT usa reloj absoluto.

Los timeouts utilizan:

```c
esp_timer_get_time()
```

No deben mezclarse.

---

# 85. Prueba manual actual

Como todavía no está conectado el ATmega definitivo, las respuestas UART se simulan mediante el monitor serie.

---

# 86. Simulación de ACK

Después de observar:

```text
<CMD,7,DISPENSE>
```

se escribe:

```text
<ACK,7>
```

---

# 87. Simulación de DONE

Posteriormente:

```text
<DONE,7>
```

---

# 88. Simulación de NACK

Ejemplo:

```text
<NACK,7,BUSY>
```

---

# 89. Resultado validado

Las pruebas demostraron:

```text
ACK
→ RECEIVED

DONE
→ SUCCESS

NACK
→ REJECTED
```

Estado:

```text
[PROBADO]
```

---

# 90. Prueba con timestamps

También se confirmó que las respuestas MQTT incluían:

```text
ts
```

con Unix timestamp sincronizado.

Estado:

```text
[PROBADO]
```

---

# 91. Prueba de ACK Timeout

Se publicó un comando desde PowerShell.

ESP32 generó:

```text
<CMD,ID,DISPENSE>
```

No se respondió manualmente.

Resultado:

```text
ACK_TIMEOUT
```

y MQTT:

```text
FAILED
```

Estado:

```text
[PROBADO]
```

---

# 92. Prueba de Execution Timeout

Se envió:

```text
<ACK,ID>
```

pero no:

```text
<DONE,ID>
```

Resultado después del tiempo configurado:

```text
EXECUTION_TIMEOUT
```

Estado:

```text
[PROBADO]
```

---

# 93. Prueba exitosa sin timeout

Se enviaron:

```text
<ACK,ID>
```

y:

```text
<DONE,ID>
```

dentro de los tiempos correspondientes.

Resultado:

```text
RECEIVED
SUCCESS
```

y no apareció posteriormente ningún timeout.

Estado:

```text
[PROBADO]
```

---

# 94. Comando desde PowerShell

Ejemplo de laboratorio:

```powershell
.\mosquitto_pub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/<DEVICE_ID>/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-000200\",\"command\":\"DISPENSE\",\"params\":{}}'
```

---

# 95. Escuchar respuestas

Rocky Linux:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/<DEVICE_ID>/command/response" \
-v
```

---

# 96. Escuchar toda la transacción

Más conveniente durante desarrollo:

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

# 97. Ejemplo de escucha completa

Puede observarse:

```text
command/request
```

```text
command/response RECEIVED
```

```text
event
```

```text
state/reported
```

```text
command/response SUCCESS
```

dependiendo de la operación.

---

# 98. Evento vs respuesta de comando

Una operación puede producir ambas cosas.

Ejemplo:

```text
DISPENSE
```

puede generar:

```text
RECEIVED
```

y después:

```text
DISPENSE_STARTED event
```

posteriormente:

```text
DISPENSE_COMPLETED event
```

y finalmente:

```text
SUCCESS
```

según el orden que defina el controlador.

---

# 99. No confundir EVENT con DONE

`EVENT` sirve para historial operativo.

`DONE` sirve para cerrar una transacción.

Son conceptos distintos.

---

# 100. Ejemplo conceptual de calibración

Servidor:

```text
START_CALIBRATION
```

ESP32:

```text
<CMD,20,START_CALIBRATION>
```

Controlador:

```text
<ACK,20>
```

MQTT:

```text
RECEIVED
```

Después:

```text
<EVENT,CALIBRATION_COMPLETED,468.2>
```

MQTT:

```text
event CALIBRATION_COMPLETED
```

Controlador:

```text
<DONE,20>
```

MQTT:

```text
SUCCESS
```

---

# 101. Resultado final vs datos del resultado

En operaciones complejas, `DONE` podría ampliarse en el futuro para incluir resultado.

Ejemplo conceptual:

```text
<DONE,20,468.2>
```

Sin embargo, la arquitectura actual prefiere separar:

```text
resultado operacional histórico
→ EVENT
```

y:

```text
cierre de transacción
→ DONE
```

---

# 102. Comandos desconocidos

Si MQTT recibe:

```text
command = UNKNOWN
```

debe responder de forma controlada.

Ejemplo:

```json
{
  "status": "REJECTED",
  "error": {
    "code": "UNKNOWN_COMMAND"
  }
}
```

sin crear una transacción UART innecesaria.

---

# 103. JSON inválido

Un payload mal formado no debe provocar ninguna operación.

Ejemplo:

```text
{command:DISPENSE
```

debe rechazarse.

---

# 104. `command_id` ausente

Ejemplo:

```json
{
  "schema": 1,
  "command": "DISPENSE",
  "params": {}
}
```

No debe ejecutarse porque no podría correlacionarse correctamente.

---

# 105. `command_id` duplicado

Este punto es importante para producción.

Con MQTT QoS 1 un mensaje puede entregarse más de una vez.

Si se recibe dos veces:

```text
CMD-000200
```

no debería provocar dos dispensados.

---

# 106. Estado actual de deduplicación

La deduplicación completa de:

```text
command_id
```

aún no se ha implementado como política persistente.

Estado:

```text
[PLANEADO]
```

---

# 107. Estrategia futura

El ESP32 deberá conservar durante cierto tiempo IDs procesados.

Conceptualmente:

```text
CMD-000200
→ ya conocido
→ no ejecutar de nuevo
```

Puede responder con el estado conocido de la transacción.

---

# 108. Idempotencia

Algunos comandos son naturalmente idempotentes.

Ejemplo:

```text
GET_STATE
```

puede ejecutarse repetidamente.

Otros no:

```text
DISPENSE
```

Por ello la deduplicación es especialmente importante para operaciones físicas.

---

# 109. Retain de comandos

Regla crítica:

```text
command/request
→ retain = false
```

Nunca debe utilizarse retained.

---

# 110. Riesgo de retain

Un comando retained podría quedar almacenado en el broker.

Cuando el dispositivo reconectara podría recibir:

```text
DISPENSE
```

como una orden vieja.

Esto es inaceptable.

---

# 111. QoS de comandos

La recomendación actual es:

```text
QoS 1
```

para:

```text
command/request
command/response
```

Esto incrementa confiabilidad de entrega, pero obliga a considerar duplicados.

---

# 112. Transacción y pérdida MQTT

Escenario:

```text
MQTT recibe comando
↓
CMD enviado al ATmega
↓
MQTT se desconecta
↓
ATmega termina
```

La operación física puede completarse aunque la respuesta MQTT no pueda publicarse inmediatamente.

---

# 113. Problema futuro

Será necesario definir una estrategia para respuestas pendientes.

Posibles opciones:

```text
cola RAM
persistencia
reintento
estado de transacción consultable
```

Estado:

```text
[PLANEADO]
```

---

# 114. Pérdida de ESP32

Si ESP32 reinicia durante una transacción:

```text
transaction_manager RAM
→ se pierde
```

El controlador podría continuar ejecutando la operación.

Esto requerirá una estrategia futura de reconciliación.

---

# 115. Posible solución futura

Después del reinicio:

```text
HELLO
↓
GET_STATE
↓
reconstruir estado
```

pero una transacción MQTT específica podría no poder recuperarse automáticamente.

Debe evaluarse según criticidad de los comandos.

---

# 116. Persistencia de transacciones

No se implementa actualmente.

Persistir cada transacción en Flash puede introducir:

```text
desgaste
complejidad
problemas de atomicidad
```

Por ello debe evaluarse antes de implementarlo.

---

# 117. Comandos largos

No todas las operaciones pueden utilizar un timeout de 30 segundos.

Ejemplos futuros:

```text
CALIBRATION
OTA
PROGRAMMING
MAINTENANCE
```

pueden tardar mucho más.

---

# 118. Timeouts específicos por comando

Futura evolución:

```text
STOP
ACK = 3 s
EXEC = 5 s

DISPENSE
ACK = 5 s
EXEC = 30 s

CALIBRATION
ACK = 5 s
EXEC = 180 s
```

Estado:

```text
[PLANEADO]
```

---

# 119. Cancelación

Actualmente existe el comando conceptual:

```text
STOP
```

pero no existe todavía un mecanismo genérico:

```text
CANCEL_TRANSACTION
```

La cancelación dependerá del tipo de operación.

---

# 120. Ejemplo STOP

Servidor:

```text
STOP
```

ESP32:

```text
<CMD,21,STOP>
```

Controlador:

```text
<ACK,21>
```

detiene operación.

Después:

```text
<DONE,21>
```

Esto constituye una transacción independiente.

---

# 121. Relación entre STOP y DISPENSE

Si:

```text
DISPENSE transaction ID 20
```

está activa y llega:

```text
STOP transaction ID 21
```

el controlador debe decidir cómo relacionarlas internamente.

El protocolo no obliga a que STOP tenga el mismo transaction ID.

---

# 122. Un comando puede modificar otro proceso

Esto pertenece a la lógica del controlador principal.

`transaction_manager` solo mantiene la relación de cada solicitud individual.

---

# 123. Errores activos vs NACK

Si existe:

```text
FLOW_SENSOR_FAIL
```

el controlador puede:

```text
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

para estado activo.

Y ante DISPENSE:

```text
<NACK,22,FLOW_SENSOR_FAIL>
```

para explicar el rechazo.

Ambos son correctos.

---

# 124. Comando local vs remoto

No todo comando MQTT debe ir a UART.

Regla:

```text
¿lo puede resolver ESP32?
→ local

¿requiere control físico?
→ controlador
```

---

# 125. Ejemplos locales futuros

Podrían existir:

```text
GET_NETWORK_INFO
RECONNECT_MQTT
GET_FIRMWARE_INFO
```

pero no están implementados todavía.

---

# 126. Ejemplos físicos futuros

```text
START_CALIBRATION
RESET_TOTAL
SET_DISPENSE_TIME
SET_COLD_SETPOINT
SET_HOT_SETPOINT
```

deberán pasar por el controlador si afectan su lógica.

---

# 127. Configuración remota

La modificación de parámetros también debe seguir transacciones.

Ejemplo futuro:

```text
SET_DISPENSE_TIME
```

ESP32:

```text
<CMD,30,SET_DISPENSE_TIME,8000>
```

Controlador:

```text
<ACK,30>
```

valida y guarda.

Después:

```text
<DONE,30>
```

---

# 128. Confirmación de configuración

Además podría emitir:

```text
<EVENT,PROGRAMMING_CHANGED,8000>
```

para registro histórico.

---

# 129. Roles de cada mensaje

```text
CMD
→ solicitar

ACK
→ aceptado

DONE
→ terminado

NACK
→ rechazado

EVENT
→ ocurrió algo

STATE
→ cómo está ahora
```

Esta distinción debe mantenerse siempre.

---

# 130. Diagnóstico mediante logs

Durante una transacción se recomienda observar logs como:

```text
Transaccion creada
UART ID=7
MQTT ID=CMD-000200
CMD=DISPENSE
```

posteriormente:

```text
ACK recibido
```

y:

```text
DONE recibido
```

o:

```text
ACK TIMEOUT
```

---

# 131. `transaction_manager_print_status()`

La función de depuración puede mostrar las transacciones activas.

Conceptualmente:

```text
SLOT 0
UART ID = 7
MQTT ID = CMD-000200
CMD = DISPENSE
STATE = ACKED
```

---

# 132. Uso de diagnóstico

Es útil para detectar:

```text
slots atascados
IDs incorrectos
estado inesperado
comandos simultáneos
```

---

# 133. No usar logs como estado

Los logs son diagnóstico.

La lógica del servidor debe basarse en:

```text
command/response
```

no en parsing de logs del ESP32.

---

# 134. Prueba completa recomendada

Para validar una transacción:

```text
1. Escuchar MQTT.
2. Enviar DISPENSE.
3. Confirmar CMD UART.
4. Enviar ACK.
5. Confirmar RECEIVED.
6. Enviar DONE.
7. Confirmar SUCCESS.
8. Esperar más de 30 s.
9. Confirmar que no aparece timeout.
```

---

# 135. Prueba NACK

```text
1. Enviar DISPENSE.
2. Leer UART ID.
3. Enviar <NACK,ID,BUSY>.
4. Confirmar REJECTED.
5. Confirmar error.code = BUSY.
6. Confirmar que la transacción se libera.
```

---

# 136. Prueba ACK Timeout

```text
1. Enviar DISPENSE.
2. No responder.
3. Esperar ~5 s.
4. Confirmar FAILED.
5. Confirmar ACK_TIMEOUT.
```

---

# 137. Prueba Execution Timeout

```text
1. Enviar DISPENSE.
2. Enviar ACK.
3. Confirmar RECEIVED.
4. No enviar DONE.
5. Esperar ~30 s.
6. Confirmar FAILED.
7. Confirmar EXECUTION_TIMEOUT.
```

---

# 138. Estado actual validado

```text
[PROBADO] command/request
[PROBADO] command/response
[PROBADO] PING local
[PROBADO] GET_STATE local
[PROBADO] creación de transacción
[PROBADO] generación UART ID
[PROBADO] CMD UART
[PROBADO] mapping MQTT ID ↔ UART ID
[PROBADO] ACK
[PROBADO] RECEIVED
[PROBADO] DONE
[PROBADO] SUCCESS
[PROBADO] NACK
[PROBADO] REJECTED
[PROBADO] timestamps
[PROBADO] ACK Timeout
[PROBADO] Execution Timeout
[PROBADO] liberación después de DONE
[PROBADO] ausencia de timeout después de éxito

[PLANEADO] deduplicación command_id
[PLANEADO] timeouts por comando
[PLANEADO] recuperación tras reboot
[PLANEADO] cola de respuestas MQTT
[PLANEADO] comandos de configuración
[PLANEADO] comandos OTA
```

---

# 139. Máquina de estados resumida

```text
                 CREAR
                   │
                   ▼
             WAITING_ACK
                /     \
               /       \
            ACK         NACK
             │            │
             ▼            ▼
           ACKED       REJECTED
             │
             │ DONE
             ▼
         COMPLETED
```

También:

```text
WAITING_ACK
    │
    │ timeout
    ▼
 FAILED
ACK_TIMEOUT
```

y:

```text
ACKED
  │
  │ timeout
  ▼
FAILED
EXECUTION_TIMEOUT
```

---

# 140. Flujo MQTT completo

```text
command/request
      │
      ▼
mqtt_manager
      │
      ▼
transaction_manager
      │
      ▼
uart_protocol
      │
      ▼
controller
      │
      ├── ACK
      ├── DONE
      └── NACK
      │
      ▼
device_manager
      │
      ▼
transaction_manager
      │
      ▼
mqtt_manager
      │
      ▼
command/response
```

---

# 141. Principios de diseño

1. Todo comando remoto debe tener `command_id`.
2. Los comandos físicos deben generar una transacción.
3. MQTT ID y UART ID son conceptos distintos.
4. ACK significa aceptación, no finalización.
5. DONE significa finalización satisfactoria.
6. NACK significa rechazo explícito.
7. FAILED representa una falla de la transacción.
8. Toda transacción debe terminar o expirar.
9. Ningún slot debe quedar ocupado indefinidamente.
10. El controlador mantiene autoridad física.
11. MQTT no debe controlar GPIO directamente.
12. Los comandos no deben utilizar retain.
13. QoS 1 obliga a considerar duplicados.
14. Un comando duplicado no debe repetir una acción física accidentalmente.
15. Eventos y respuestas de comandos son conceptos distintos.
16. Los errores activos y los errores de transacción son conceptos distintos.
17. Las operaciones largas requerirán timeouts específicos.
18. El sistema debe poder diagnosticar transacciones activas.
19. Las respuestas tardías no deben reabrir transacciones terminadas.
20. La arquitectura debe mantenerse independiente del modelo del equipo.

---

# 142. Relación con otros documentos

Protocolo MQTT:

```text
08_PROTOCOLO_MQTT.md
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

Timeouts:

```text
13_TIMEOUTS.md
```

Pruebas:

```text
14_PRUEBAS_Y_DIAGNOSTICO.md
```

---

# 143. Resumen

El sistema de comandos y transacciones constituye la ruta **servidor → equipo → servidor** de MAIM Connectivity.

Su funcionamiento puede resumirse como:

```text
Servidor
→ solicita una operación

MQTT
→ transporta command/request

ESP32
→ crea una transacción

Transaction Manager
→ asigna UART ID

UART
→ transmite CMD

Controlador
→ valida físicamente

ACK
→ aceptado

DONE
→ completado

NACK
→ rechazado

Timeout
→ controlador no respondió correctamente

MQTT
→ informa resultado al servidor
```

Con esta arquitectura, el servidor no solo sabe que envió una solicitud: puede saber si el controlador **la recibió, la aceptó, la rechazó, la terminó o dejó de responder**.

Esto constituye la base transaccional de **MAIM Connectivity v0.1.0**.

---

**Fin del documento — `12_COMANDOS_Y_TRANSACCIONES.md`**
