# MAIM Connectivity

## Sistema de Eventos

**Documento:** `11_EVENTOS.md`
**Proyecto:** MAIM Connectivity
**Arquitectura:** ESP32 + controlador principal
**Protocolo UART:** MAIM Internal UART Protocol v1
**Protocolo MQTT:** MAIM MQTT Protocol v1
**Estado:** [PROBADO] Ruta inmediata UART → ESP32 → MQTT funcional

---

# 1. Propósito

El sistema de eventos permite informar al servidor sobre **acciones o sucesos puntuales ocurridos dentro del equipo**.

Un evento representa:

> **Algo ocurrió en un momento determinado.**

Ejemplos:

```text
DISPENSE_STARTED
DISPENSE_COMPLETED
CALIBRATION_COMPLETED
PROGRAMMING_CHANGED
```

A diferencia del estado actual del equipo, los eventos representan acontecimientos históricos y deben poder almacenarse posteriormente en el servidor para análisis, auditoría, métricas y diagnóstico.

---

# 2. Diferencia entre estado y evento

Esta distinción es fundamental.

## Estado

Un estado representa:

> **Cómo se encuentra el equipo actualmente.**

Ejemplo UART:

```text
<STATE,MODE,DISPENSING>
```

Representación:

```text
MODE = DISPENSING
```

Puede cambiar posteriormente:

```text
<STATE,MODE,STANDBY>
```

Por lo tanto, normalmente solo interesa conservar el **valor actual**.

---

## Evento

Un evento representa:

> **Algo que ocurrió.**

Ejemplo:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

Este evento no sustituye al anterior.

Cada dispensado genera un nuevo acontecimiento independiente.

---

# 3. Ejemplo conceptual

Si un usuario dispensa agua tres veces:

```text
DISPENSE_COMPLETED
DISPENSE_COMPLETED
DISPENSE_COMPLETED
```

tenemos tres eventos distintos.

Sin embargo, una métrica como:

```text
WATER_TOTAL_ML
```

simplemente cambia:

```text
16000
→ 16287
→ 16550
→ 16820
```

Por ello:

```text
STATE / METRIC
→ fotografía actual

EVENT
→ historial de acontecimientos
```

---

# 4. Diferencia entre EVENT y ERROR

Un error representa una **condición activa**.

Ejemplo:

```text
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

Mientras el problema exista:

```text
FLOW_SENSOR_FAIL
```

forma parte del estado actual del equipo.

Cuando desaparece:

```text
<ERROR_CLEAR,FLOW_SENSOR_FAIL>
```

se elimina de los errores activos.

Un evento, en cambio, representa algo que ocurrió y no necesita permanecer activo.

---

# 5. Clasificación conceptual

La arquitectura diferencia:

| Tipo     | Significado               | Persistencia en Device Manager | Publicación                  |
| -------- | ------------------------- | -----------------------------: | ---------------------------- |
| `STATE`  | Estado actual             |                             Sí | `state/reported`             |
| `METRIC` | Valor acumulado/operativo |                             Sí | `state/reported` / telemetry |
| `SENSOR` | Lectura actual            |                             Sí | state / telemetry            |
| `OUTPUT` | Estado de salida          |                             Sí | `state/reported`             |
| `ERROR`  | Error actualmente activo  |                             Sí | state / eventos futuros      |
| `EVENT`  | Suceso puntual            |                             No | inmediata                    |
| `ACK`    | Confirmación transacción  |                             No | command response             |
| `DONE`   | Finalización transacción  |                             No | command response             |
| `NACK`   | Rechazo transacción       |                             No | command response             |

---

# 6. Arquitectura de eventos

La ruta implementada actualmente es:

```text
Controlador principal
        │
        │ UART
        ▼
   uart_protocol
        │
        ▼
  device_manager
        │
        │ callback
        ▼
   mqtt_manager
        │
        ▼
   MQTT Broker
        │
        ▼
      Servidor
```

Esto permite que los eventos sean publicados inmediatamente.

---

# 7. Principio de publicación inmediata

Los eventos **no esperan al siguiente ciclo de telemetría**.

Ejemplo:

```text
10:00:00
telemetry

10:00:05
DISPENSE_COMPLETED
```

El ESP32 no debe esperar hasta:

```text
10:00:30
```

para informar el dispensado.

Debe publicar el evento aproximadamente cuando ocurre:

```text
10:00:05
```

---

# 8. Razón de la publicación inmediata

Esto permite:

* historial preciso;
* respuesta rápida del servidor;
* registro de acciones;
* estadísticas de uso;
* diagnóstico;
* futuras notificaciones;
* auditoría;
* automatizaciones.

---

# 9. Formato UART de EVENT

El formato general es:

```text
<EVENT,EVENT_TYPE[,PARAM1,PARAM2,...]>
```

Ejemplo simple:

```text
<EVENT,DISPENSE_STARTED>
```

Ejemplo con parámetros:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

---

# 10. EVENT_TYPE

El primer campo después de `EVENT` identifica el acontecimiento.

Ejemplo:

```text
<EVENT,DISPENSE_STARTED>
```

produce:

```text
EVENT_TYPE = DISPENSE_STARTED
```

---

# 11. Parámetros

Los campos posteriores dependen del tipo de evento.

Ejemplo:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

significa actualmente:

```text
FIELD[0]
DISPENSE_COMPLETED

FIELD[1]
287

FIELD[2]
16287
```

La interpretación semántica es:

```text
287
→ volumen del dispensado en ml

16287
→ volumen total acumulado en ml
```

---

# 12. Responsabilidad de `uart_protocol`

`uart_protocol` solamente determina:

```text
TYPE = EVENT

FIELDS:
DISPENSE_COMPLETED
287
16287
```

No debe conocer qué significa:

```text
287
```

o:

```text
16287
```

---

# 13. Responsabilidad de `device_manager`

`device_manager` identifica que se trata de un evento y ejecuta el callback registrado.

Conceptualmente:

```text
EVENT recibido
      ↓
validación básica
      ↓
event_callback()
```

El evento no se almacena permanentemente dentro de las tablas del Device Manager.

---

# 14. Callback de eventos

Conceptualmente:

```c
typedef void (*device_manager_event_callback_t)(
    const char *event_type,
    const uart_protocol_frame_t *frame
);
```

Esto permite desacoplar:

```text
device_manager
```

de:

```text
mqtt_manager
```

---

# 15. Flujo completo

Ejemplo:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

## Paso 1 — UART Protocol

Interpreta:

```text
TYPE = EVENT

FIELD[0] = DISPENSE_COMPLETED
FIELD[1] = 287
FIELD[2] = 16287
```

## Paso 2 — Device Manager

Determina:

```text
event_type = DISPENSE_COMPLETED
```

y ejecuta el callback.

## Paso 3 — MQTT Manager

Convierte el evento al formato JSON correspondiente.

## Paso 4 — Broker

Publica:

```text
maim/v1/devices/MM_TEST_001/event
```

---

# 16. Topic MQTT

Todos los eventos actuales del dispositivo utilizan:

```text
maim/v1/devices/{device_id}/event
```

Para el dispositivo de pruebas:

```text
maim/v1/devices/MM_TEST_001/event
```

---

# 17. Estructura MQTT

La estructura general implementada es:

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

# 18. Campo `schema`

```json
"schema": 1
```

identifica la versión del esquema MQTT utilizado para representar el evento.

Esto permitirá modificar la estructura en el futuro manteniendo compatibilidad.

---

# 19. Campo `event_id`

Cada evento recibe un identificador.

Formato actual:

```text
EVT-{DEVICE_ID}-{SEQUENCE}
```

Ejemplo:

```text
EVT-MM_TEST_001-000001
```

---

# 20. Ejemplo de secuencia

```text
EVT-MM_TEST_001-000001
EVT-MM_TEST_001-000002
EVT-MM_TEST_001-000003
EVT-MM_TEST_001-000004
```

Esto permite distinguir eventos individuales.

---

# 21. Función del `event_id`

El identificador permitirá posteriormente:

* almacenar eventos;
* evitar duplicados;
* rastrear eventos;
* depurar comunicaciones;
* relacionar registros;
* implementar auditoría.

---

# 22. Secuencia actual

Actualmente el ESP32 mantiene un contador interno para generar:

```text
000001
000002
000003
...
```

La estrategia definitiva de persistencia del contador deberá revisarse posteriormente.

---

# 23. Reinicio del ESP32

Si el contador vive únicamente en RAM:

```text
ESP32 reboot
```

puede provocar que la secuencia vuelva a comenzar.

Por ello, `event_id` todavía no debe considerarse un identificador globalmente único permanente.

Estado:

```text
[POR DEFINIR]
```

---

# 24. Posibles estrategias futuras

Podrían utilizarse:

```text
DEVICE_ID + UNIX TIMESTAMP + COUNTER
```

o:

```text
UUID
```

o persistencia del contador en NVS.

Debe evaluarse antes de implementar almacenamiento histórico definitivo.

---

# 25. Campo `ts`

```json
"ts": 1786504167
```

representa el timestamp Unix generado por el ESP32.

La hora proviene de:

```text
time_manager
```

después de sincronización SNTP.

---

# 26. Importancia del timestamp

El timestamp permite conocer aproximadamente cuándo ocurrió el evento.

Esto será fundamental para:

* históricos;
* gráficas;
* estadísticas;
* mantenimiento;
* auditoría;
* correlación de fallas.

---

# 27. Relación temporal ATmega → ESP32

Actualmente el controlador principal no genera el timestamp Unix.

El flujo es:

```text
evento ocurre en controlador
        ↓
trama UART
        ↓
ESP32 recibe evento
        ↓
ESP32 obtiene timestamp
        ↓
MQTT
```

Por ello el timestamp representa prácticamente:

> **el momento en que el ESP32 procesa/publica el evento.**

---

# 28. Precisión temporal

Normalmente la diferencia entre:

```text
evento físico
```

y:

```text
timestamp MQTT
```

será muy pequeña.

Sin embargo, conceptualmente no son exactamente lo mismo.

Esto debe considerarse si en el futuro se requiere precisión temporal estricta.

---

# 29. Campo `type`

Ejemplo:

```json
"type": "DISPENSE_COMPLETED"
```

identifica el tipo de evento.

Los nombres deben mantenerse estables porque serán utilizados posteriormente por:

```text
backend
base de datos
dashboard
analytics
alertas
```

---

# 30. Convención de nombres

Los eventos utilizan:

```text
UPPER_SNAKE_CASE
```

Ejemplos:

```text
DISPENSE_STARTED
DISPENSE_COMPLETED
CALIBRATION_COMPLETED
PROGRAMMING_CHANGED
```

---

# 31. No cambiar nombres arbitrariamente

Una vez que un evento sea utilizado por producción:

```text
DISPENSE_COMPLETED
```

no debe cambiarse posteriormente a:

```text
DISPENSE_FINISHED
```

sin modificar la versión del protocolo o implementar compatibilidad.

---

# 32. Campo `severity`

Ejemplo:

```json
"severity": "info"
```

clasifica la importancia del evento.

Para eventos operativos normales actualmente se utiliza:

```text
info
```

---

# 33. Severidades MQTT

La arquitectura contempla conceptualmente:

```text
info
warning
error
critical
```

Estas severidades permiten al servidor decidir posteriormente qué eventos:

* almacenar;
* destacar;
* notificar;
* convertir en alertas.

---

# 34. EVENT no significa ERROR

Por ejemplo:

```text
DISPENSE_COMPLETED
```

es:

```text
severity = info
```

Un futuro evento:

```text
WATER_LEAK_DETECTED
```

podría utilizar:

```text
severity = critical
```

aunque la condición activa también pueda existir dentro de:

```text
ERROR
```

---

# 35. Evento de aparición de error

En una evolución futura podría existir:

```text
ERROR_RAISED
```

cuando aparece un error.

Esto permitiría tener:

```text
estado actual:
FLOW_SENSOR_FAIL activo
```

y simultáneamente:

```text
historial:
FLOW_SENSOR_FAIL apareció a las 10:32
```

---

# 36. Evento de eliminación de error

También podría existir:

```text
ERROR_CLEARED
```

para registrar cuándo se recuperó el equipo.

Estado:

```text
[PLANEADO]
```

---

# 37. Campo `data`

Todos los datos específicos del evento se colocan dentro de:

```json
"data": {}
```

Esto mantiene una estructura MQTT uniforme.

---

# 38. Evento sin parámetros

Ejemplo:

```text
<EVENT,DISPENSE_STARTED>
```

produce:

```json
{
  "type": "DISPENSE_STARTED",
  "data": {}
}
```

---

# 39. Evento con parámetros

Ejemplo:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

produce:

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

# 40. Ventaja del objeto `data`

Evita estructuras como:

```json
{
  "param1": 287,
  "param2": 16287
}
```

que serían difíciles de interpretar.

MQTT utiliza nombres semánticos:

```json
"volume_ml"
"water_total_ml"
```

---

# 41. Conversión UART → MQTT

UART puede permanecer compacto:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

mientras MQTT puede ser descriptivo:

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

# 42. Razón de esta arquitectura

UART debe priorizar:

```text
simplicidad
pocos bytes
facilidad de implementación en ATmega
```

MQTT debe priorizar:

```text
claridad
semántica
extensibilidad
facilidad para backend
```

---

# 43. Eventos actualmente implementados

Durante las pruebas se implementaron y verificaron:

```text
DISPENSE_STARTED
DISPENSE_COMPLETED
CALIBRATION_COMPLETED
PROGRAMMING_CHANGED
```

---

# 44. DISPENSE_STARTED

Representa:

> Inicio de una operación de dispensado.

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

# 45. Uso de DISPENSE_STARTED

Puede utilizarse posteriormente para:

* actividad en tiempo real;
* medir duración del dispensado;
* detectar dispensados interrumpidos;
* telemetría operacional.

---

# 46. DISPENSE_COMPLETED

Representa:

> Un dispensado terminó correctamente.

UART:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

Parámetros:

```text
287
→ volumen dispensado

16287
→ volumen acumulado
```

---

# 47. MQTT DISPENSE_COMPLETED

Resultado probado:

```json
{
  "schema": 1,
  "event_id": "EVT-MM_TEST_001-000002",
  "ts": 1786504208,
  "type": "DISPENSE_COMPLETED",
  "severity": "info",
  "data": {
    "volume_ml": 287,
    "water_total_ml": 16287
  }
}
```

Estado:

```text
[PROBADO]
```

---

# 48. Importancia de DISPENSE_COMPLETED

Este evento permitirá obtener estadísticas como:

```text
dispensados por día
dispensados por equipo
ml por dispensado
litros por periodo
horarios de mayor uso
promedio de consumo
```

sin depender únicamente de la métrica acumulada.

---

# 49. CALIBRATION_COMPLETED

Representa:

> Finalización satisfactoria del proceso de calibración del caudalímetro.

UART:

```text
<EVENT,CALIBRATION_COMPLETED,468.2>
```

---

# 50. MQTT CALIBRATION_COMPLETED

Resultado probado:

```json
{
  "schema": 1,
  "event_id": "EVT-MM_TEST_001-000003",
  "ts": 1786504244,
  "type": "CALIBRATION_COMPLETED",
  "severity": "info",
  "data": {
    "pulses_per_liter": 468.20001220703125
  }
}
```

Estado:

```text
[PROBADO]
```

---

# 51. Precisión float observada

Durante la prueba UART se envió aproximadamente:

```text
468.2
```

pero JSON mostró:

```text
468.20001220703125
```

Esto es consecuencia de la representación binaria de `float`.

No representa necesariamente un error de calibración.

---

# 52. Mejora futura de precisión

Podría evaluarse:

* redondear al generar JSON;
* utilizar enteros escalados;
* definir precisión fija por variable.

Ejemplo:

```text
46820
```

podría representar:

```text
468.20
```

si el protocolo define una escala de ×100.

Estado:

```text
[POR EVALUAR]
```

---

# 53. PROGRAMMING_CHANGED

Representa:

> El usuario modificó un parámetro de programación del equipo.

Durante las pruebas se utilizó:

```text
dispense_time_ms
```

---

# 54. UART PROGRAMMING_CHANGED

Ejemplo:

```text
<EVENT,PROGRAMMING_CHANGED,8000>
```

---

# 55. MQTT PROGRAMMING_CHANGED

Resultado probado:

```json
{
  "schema": 1,
  "event_id": "EVT-MM_TEST_001-000004",
  "ts": 1786504282,
  "type": "PROGRAMMING_CHANGED",
  "severity": "info",
  "data": {
    "dispense_time_ms": 8000
  }
}
```

Estado:

```text
[PROBADO]
```

---

# 56. Prueba realizada

Se enviaron manualmente mediante la terminal UART:

```text
<EVENT,DISPENSE_STARTED>
```

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

```text
<EVENT,CALIBRATION_COMPLETED,468.2>
```

```text
<EVENT,PROGRAMMING_CHANGED,8000>
```

---

# 57. Resultado observado en Mosquitto

El broker recibió:

```text
maim/v1/devices/MM_TEST_001/event {"schema":1,"event_id":"EVT-MM_TEST_001-000001","ts":1786504167,"type":"DISPENSE_STARTED","severity":"info","data":{}}
```

```text
maim/v1/devices/MM_TEST_001/event {"schema":1,"event_id":"EVT-MM_TEST_001-000002","ts":1786504208,"type":"DISPENSE_COMPLETED","severity":"info","data":{"volume_ml":287,"water_total_ml":16287}}
```

```text
maim/v1/devices/MM_TEST_001/event {"schema":1,"event_id":"EVT-MM_TEST_001-000003","ts":1786504244,"type":"CALIBRATION_COMPLETED","severity":"info","data":{"pulses_per_liter":468.20001220703125}}
```

```text
maim/v1/devices/MM_TEST_001/event {"schema":1,"event_id":"EVT-MM_TEST_001-000004","ts":1786504282,"type":"PROGRAMMING_CHANGED","severity":"info","data":{"dispense_time_ms":8000}}
```

La ruta completa quedó confirmada.

---

# 58. Resultado de validación

```text
UART manual
    ↓
uart_protocol
    ↓
device_manager
    ↓
callback
    ↓
mqtt_manager
    ↓
Mosquitto
```

Estado:

```text
[PROBADO]
```

---

# 59. Eventos y Device Manager

Los eventos **no deben agregarse a las tablas permanentes del Device Manager**.

Por ejemplo:

```text
DISPENSE_COMPLETED
```

no debe quedar como:

```text
EVENT = DISPENSE_COMPLETED
```

porque perderíamos los eventos anteriores.

---

# 60. Eventos y MQTT retained

El topic:

```text
/event
```

**no debe utilizar mensajes retained**.

Un evento es histórico, no representa el estado actual.

---

# 61. Razón para no utilizar retained

Si:

```text
DISPENSE_COMPLETED
```

fuera retained, un cliente que se conectara horas después podría recibirlo como si acabara de ocurrir.

Esto sería incorrecto.

---

# 62. Retained vs eventos

Regla:

```text
state/reported
→ puede utilizar retained

availability
→ puede utilizar retained según estrategia

event
→ NO retained
```

---

# 63. QoS

Los eventos actuales pueden publicarse utilizando:

```text
QoS 1
```

para aumentar la confiabilidad de entrega.

Esto significa:

> **At least once.**

---

# 64. Implicación de QoS 1

Un evento podría llegar más de una vez en ciertas circunstancias.

Por ello el backend deberá poder manejar duplicados.

Aquí cobra importancia:

```text
event_id
```

---

# 65. Deduplificación futura

El servidor podría utilizar:

```text
device_id + event_id
```

para detectar un evento previamente almacenado.

Esto requiere que `event_id` tenga una estrategia suficientemente robusta entre reinicios.

---

# 66. MQTT desconectado

Existe un problema arquitectónico importante:

```text
EVENT ocurre
↓
MQTT desconectado
```

Actualmente el evento podría no llegar al servidor dependiendo de la implementación.

---

# 67. Cola de eventos futura

Se recomienda implementar posteriormente:

```text
Event Queue
```

para mantener temporalmente eventos pendientes.

Flujo:

```text
EVENT
 ↓
queue
 ↓
MQTT disponible?
 ├── sí → publicar
 └── no → conservar temporalmente
```

Estado:

```text
[PLANEADO]
```

---

# 68. Persistencia de eventos críticos

Para eventos especialmente importantes podría evaluarse almacenamiento temporal en:

```text
NVS
```

o una partición dedicada.

No debe implementarse todavía sin analizar desgaste de Flash y requisitos reales.

---

# 69. Eventos normales vs críticos

No todos los eventos necesitan persistencia offline.

Ejemplo:

```text
DISPENSE_STARTED
```

podría ser menos importante que:

```text
LEAK_DETECTED
```

La política deberá definirse posteriormente.

---

# 70. Política futura sugerida

Conceptualmente:

```text
INFO
→ RAM queue

WARNING
→ RAM queue

ERROR
→ RAM queue + posible persistencia

CRITICAL
→ persistencia hasta confirmación
```

Estado:

```text
[DISEÑO FUTURO]
```

---

# 71. Eventos desconocidos

Si el controlador envía:

```text
<EVENT,NEW_EVENT,123>
```

el sistema debe evitar fallar completamente.

Dependiendo de la implementación MQTT, puede:

```text
registrar evento desconocido
```

o:

```text
rechazar mapping desconocido
```

pero nunca bloquear el procesamiento UART.

---

# 72. Filosofía de extensibilidad

Agregar un nuevo evento debe requerir principalmente:

```text
1. Definir nombre UART
2. Definir parámetros
3. Definir mapping MQTT
4. Definir severity
5. Documentarlo
6. Probarlo
```

No debe requerir modificar toda la arquitectura.

---

# 73. Catálogo formal de eventos

A medida que el proyecto crezca será necesario mantener un catálogo.

Ejemplo:

| Evento                  | Parámetros UART           | Severity | Estado  |
| ----------------------- | ------------------------- | -------- | ------- |
| `DISPENSE_STARTED`      | ninguno                   | info     | PROBADO |
| `DISPENSE_COMPLETED`    | volume_ml, water_total_ml | info     | PROBADO |
| `CALIBRATION_COMPLETED` | pulses_per_liter          | info     | PROBADO |
| `PROGRAMMING_CHANGED`   | dispense_time_ms          | info     | PROBADO |

---

# 74. Nuevos modelos MAIM

Otros equipos podrán agregar eventos como:

```text
COLD_TANK_TARGET_REACHED
HOT_TANK_TARGET_REACHED
ECO_MODE_ENABLED
ECO_MODE_DISABLED
SABBATH_MODE_ENABLED
FILTER_CHANGED
MAINTENANCE_COMPLETED
LEAK_DETECTED
NO_WATER_DETECTED
```

Estos nombres son ejemplos de arquitectura futura y **todavía no forman parte del protocolo implementado**.

---

# 75. No utilizar eventos para telemetría continua

Incorrecto:

```text
<EVENT,TEMPERATURE,7.8>
<EVENT,TEMPERATURE,7.9>
<EVENT,TEMPERATURE,8.0>
```

Para eso existe:

```text
SENSOR
```

y:

```text
telemetry
```

---

# 76. No utilizar eventos para estado continuo

Incorrecto:

```text
<EVENT,VALVE,1>
```

Si lo que se desea conocer es el estado actual de la válvula debe utilizarse:

```text
<OUTPUT,VALVE,1>
```

---

# 77. Sí utilizar evento para cambio significativo

Puede tener sentido utilizar simultáneamente:

```text
<OUTPUT,VALVE,1>
```

y:

```text
<EVENT,DISPENSE_STARTED>
```

porque representan conceptos diferentes.

---

# 78. Actualización de estado + evento

Ejemplo de dispensado:

```text
<STATE,MODE,DISPENSING>
<STATE,BUSY,1>
<OUTPUT,VALVE,1>
<EVENT,DISPENSE_STARTED>
```

Aquí:

```text
STATE / OUTPUT
→ situación actual
```

y:

```text
EVENT
→ registro histórico
```

---

# 79. Final de dispensado

Ejemplo:

```text
<OUTPUT,VALVE,0>
<STATE,BUSY,0>
<STATE,MODE,STANDBY>
<METRIC,LAST_DISPENSE_ML,287>
<METRIC,WATER_TOTAL_ML,16287>
<METRIC,DISPENSE_COUNT,49>
<EVENT,DISPENSE_COMPLETED,287,16287>
```

Esto mantiene sincronizados:

```text
estado
métricas
histórico
```

---

# 80. Orden UART

Cuando varios mensajes representan la misma operación, debe mantenerse un orden lógico.

Preferentemente:

```text
actualizar estado/métricas
        ↓
emitir evento
```

Así, cuando el servidor reciba:

```text
DISPENSE_COMPLETED
```

el estado interno del ESP32 ya contiene los valores actualizados.

---

# 81. EVENT y GET_STATE

Después de recibir:

```text
<EVENT,DISPENSE_COMPLETED,...>
```

si el servidor ejecuta inmediatamente:

```text
GET_STATE
```

debe recibir las métricas actualizadas si el controlador las envió antes del evento.

---

# 82. Importancia del orden

Incorrecto:

```text
EVENT DISPENSE_COMPLETED
↓
servidor GET_STATE
↓
métrica todavía antigua
```

Correcto:

```text
actualizar métricas
↓
EVENT DISPENSE_COMPLETED
↓
servidor GET_STATE
↓
métricas nuevas
```

---

# 83. Responsabilidad del controlador

El controlador principal debe emitir los eventos en el momento correcto.

El ESP32 no debe intentar deducir acontecimientos complejos a partir de estados si el controlador ya conoce exactamente cuándo ocurrieron.

---

# 84. Principio

> **El controlador físico detecta el acontecimiento; el ESP32 lo transporta y enriquece para la red.**

---

# 85. Enriquecimiento del ESP32

UART:

```text
<EVENT,DISPENSE_STARTED>
```

ESP32 agrega:

```text
schema
event_id
timestamp
severity
nombres JSON
device_id implícito en topic
```

---

# 86. Beneficio

El ATmega no necesita implementar:

```text
JSON
MQTT
Unix timestamps
WiFi
event IDs complejos
```

Esto reduce significativamente su carga.

---

# 87. Responsabilidad del servidor

El servidor deberá posteriormente:

```text
recibir
validar
deduplicar
almacenar
clasificar
consultar
graficar
```

los eventos.

Esta parte todavía no forma parte del firmware actual.

---

# 88. Base de datos futura

Los eventos probablemente terminarán en una tabla conceptual similar a:

```text
device_events
```

con:

```text
device_id
event_id
timestamp
type
severity
data
```

Estado:

```text
[PLANEADO]
```

---

# 89. JSON `data`

El campo:

```json
"data": {}
```

permite que distintos eventos tengan distinta información sin cambiar la estructura principal de la tabla o mensaje.

---

# 90. Ejemplo futuro

```json
{
  "schema": 1,
  "event_id": "...",
  "ts": 1786505000,
  "type": "FILTER_CHANGED",
  "severity": "info",
  "data": {
    "previous_liters": 15230,
    "technician_id": "..."
  }
}
```

Este ejemplo es únicamente conceptual.

---

# 91. Validación de parámetros

Cada evento conocido debe validar:

```text
cantidad de parámetros
tipo de parámetros
rango cuando corresponda
```

Ejemplo:

```text
DISPENSE_COMPLETED
```

espera:

```text
volume_ml
water_total_ml
```

---

# 92. Evento incompleto

Ejemplo:

```text
<EVENT,DISPENSE_COMPLETED,287>
```

no contiene toda la información esperada.

Debe registrarse como evento inválido o rechazarse según la política del mapping.

---

# 93. Evento con dato inválido

Ejemplo:

```text
<EVENT,DISPENSE_COMPLETED,ABC,16287>
```

no debe convertirse silenciosamente a:

```text
volume_ml = 0
```

Debe detectarse la conversión inválida.

---

# 94. Seguridad ante datos incorrectos

Nunca asumir:

```text
atoi() == dato válido
```

sin comprobar la conversión.

Debe utilizarse validación equivalente a la empleada en Device Manager.

---

# 95. Logs

Durante desarrollo deben generarse logs útiles como:

```text
EVENT recibido: DISPENSE_COMPLETED
```

```text
EVENT publicado: DISPENSE_COMPLETED
```

o errores como:

```text
EVENT invalido: parametros insuficientes
```

---

# 96. Evitar exceso de logs

En producción deberá evaluarse el nivel de:

```text
ESP_LOGI
ESP_LOGW
ESP_LOGE
```

para evitar llenar innecesariamente la salida serie.

---

# 97. Evento publicado correctamente

La publicación MQTT exitosa significa:

```text
mensaje entregado al cliente MQTT para envío
```

y con QoS 1 posteriormente existe confirmación MQTT del broker.

Esto no significa necesariamente que la aplicación backend ya haya procesado el evento.

---

# 98. Confirmación de aplicación

Si en el futuro se requiere saber que el servidor almacenó el evento, sería necesario implementar un mecanismo adicional de:

```text
application ACK
```

No está implementado actualmente.

---

# 99. Eventos durante reboot

Eventos ocurridos mientras el ESP32 está completamente apagado no pueden ser enviados por el ESP32.

Si esto debe resolverse en el futuro, el controlador tendría que mantener:

```text
event buffer
```

o contadores suficientes para reconstruir información.

---

# 100. Eventos durante pérdida WiFi

Situación:

```text
ATmega funcionando
ESP32 funcionando
WiFi caído
```

Los eventos pueden llegar correctamente por UART pero MQTT no puede enviarlos.

Este escenario justifica la futura:

```text
event queue
```

---

# 101. Eventos durante pérdida MQTT

También puede existir:

```text
WiFi conectado
MQTT desconectado
```

El comportamiento debe ser equivalente:

```text
evento pendiente
```

si posteriormente se implementa la cola.

---

# 102. Límite de cola futuro

La cola deberá tener un tamaño máximo.

No puede crecer indefinidamente.

Deberá definirse una política:

```text
FIFO
```

y comportamiento cuando esté llena.

---

# 103. Prioridad futura

Una posible política:

```text
CRITICAL
→ nunca descartar mientras sea posible

ERROR
→ alta prioridad

WARNING
→ prioridad media

INFO
→ descartable si no existe espacio
```

Todavía no implementado.

---

# 104. Eventos y OTA

Durante una actualización OTA deberá definirse qué ocurre con eventos pendientes.

Si existe una cola persistente:

```text
OTA
→ reboot
→ recuperar eventos
```

podría conservarlos.

Estado:

```text
[PLANEADO]
```

---

# 105. Compatibilidad de protocolo

Agregar un nuevo tipo de evento normalmente no requiere aumentar:

```text
schema
```

si mantiene la estructura:

```json
{
  "schema": 1,
  "event_id": "...",
  "ts": 0,
  "type": "...",
  "severity": "...",
  "data": {}
}
```

---

# 106. Cuándo cambiar schema

Debe considerarse aumentar el schema si cambia estructuralmente:

```text
event_id
timestamp
severity
data
```

o su semántica fundamental.

---

# 107. Backward compatibility

El servidor debe idealmente ignorar tipos de eventos desconocidos sin fallar.

Esto permitirá desplegar firmware nuevo antes de actualizar todos los consumidores.

---

# 108. Regla de robustez

> Un evento desconocido no debe provocar que el backend deje de procesar eventos conocidos.

La misma filosofía aplica al ESP32.

---

# 109. Ejemplo de operación completa

Inicio:

```text
<STATE,MODE,DISPENSING>
<STATE,BUSY,1>
<OUTPUT,VALVE,1>
<EVENT,DISPENSE_STARTED>
```

MQTT inmediato:

```json
{
  "type": "DISPENSE_STARTED",
  "severity": "info",
  "data": {}
}
```

---

# 110. Finalización

Posteriormente:

```text
<OUTPUT,VALVE,0>
<STATE,BUSY,0>
<STATE,MODE,STANDBY>
<METRIC,LAST_DISPENSE_ML,287>
<METRIC,WATER_TOTAL_ML,16287>
<METRIC,DISPENSE_COUNT,49>
<EVENT,DISPENSE_COMPLETED,287,16287>
```

---

# 111. Resultado

El servidor obtiene simultáneamente:

```text
estado actual
+
métricas actuales
+
histórico de eventos
```

sin mezclar los tres conceptos.

---

# 112. Ventaja para análisis futuro

A partir de eventos de dispensado será posible calcular independientemente:

```text
número de servicios
volumen promedio
horarios
frecuencia
duración
consumo diario
consumo mensual
```

---

# 113. Ventaja para mantenimiento

Eventos como:

```text
CALIBRATION_COMPLETED
PROGRAMMING_CHANGED
```

permitirán conocer cuándo se modificó un parámetro importante.

---

# 114. Auditoría futura

Con eventos adicionales podrá conocerse:

```text
qué ocurrió
cuándo ocurrió
en qué equipo
con qué parámetros
```

Esto será especialmente útil cuando el sistema se despliegue en múltiples equipos.

---

# 115. Eventos que NO están implementados todavía

No asumir como existentes:

```text
ERROR_RAISED
ERROR_CLEARED
BOOT_COMPLETED
CONTROLLER_REBOOTED
WIFI_LOST
MQTT_LOST
OTA_STARTED
OTA_COMPLETED
LEAK_DETECTED
NO_WATER_DETECTED
```

Son posibles extensiones futuras.

---

# 116. Eventos de infraestructura

En el futuro deberá decidirse si eventos como:

```text
WiFi conectado
MQTT desconectado
ESP32 reiniciado
OTA realizada
```

utilizarán el mismo topic:

```text
/event
```

o un subsistema específico de diagnóstico.

Estado:

```text
[POR DEFINIR]
```

---

# 117. Pruebas manuales

Durante desarrollo, la entrada UART puede simularse desde el monitor serie.

Ejemplo:

```text
<EVENT,DISPENSE_STARTED>
```

Después debe verificarse el broker.

---

# 118. Escucha MQTT

En el servidor puede utilizarse:

```bash
mosquitto_sub -h localhost -p 1883 -u USUARIO -P CONTRASEÑA -t 'maim/v1/devices/MM_TEST_001/event' -v
```

También puede escucharse todo el dispositivo:

```bash
mosquitto_sub -h localhost -p 1883 -u USUARIO -P CONTRASEÑA -t 'maim/v1/devices/MM_TEST_001/#' -v
```

No deben almacenarse credenciales reales dentro de esta documentación.

---

# 119. Resultado esperado

Después de:

```text
<EVENT,DISPENSE_STARTED>
```

debe aparecer aproximadamente:

```text
maim/v1/devices/MM_TEST_001/event {"schema":1,...,"type":"DISPENSE_STARTED","severity":"info","data":{}}
```

---

# 120. Prueba DISPENSE_COMPLETED

Enviar:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

Debe observarse:

```json
{
  "type": "DISPENSE_COMPLETED",
  "severity": "info",
  "data": {
    "volume_ml": 287,
    "water_total_ml": 16287
  }
}
```

---

# 121. Prueba CALIBRATION_COMPLETED

Enviar:

```text
<EVENT,CALIBRATION_COMPLETED,468.2>
```

Debe observarse:

```json
{
  "type": "CALIBRATION_COMPLETED",
  "severity": "info",
  "data": {
    "pulses_per_liter": 468.2
  }
}
```

La representación exacta del float puede contener decimales adicionales.

---

# 122. Prueba PROGRAMMING_CHANGED

Enviar:

```text
<EVENT,PROGRAMMING_CHANGED,8000>
```

Debe producir:

```json
{
  "type": "PROGRAMMING_CHANGED",
  "severity": "info",
  "data": {
    "dispense_time_ms": 8000
  }
}
```

---

# 123. Checklist para agregar un evento

Al agregar un evento nuevo:

1. Definir nombre en `UPPER_SNAKE_CASE`.
2. Definir significado exacto.
3. Definir parámetros UART.
4. Definir unidades.
5. Definir tipos numéricos.
6. Definir `severity`.
7. Agregar mapping UART → JSON.
8. Definir estructura `data`.
9. Probar entrada UART.
10. Verificar publicación MQTT.
11. Verificar timestamp.
12. Verificar `event_id`.
13. Documentarlo.
14. Verificar compatibilidad con backend.

---

# 124. Regla de unidades

Los nombres JSON deben incluir unidades cuando sea útil.

Preferir:

```text
volume_ml
dispense_time_ms
pulses_per_liter
```

en lugar de:

```text
volume
time
calibration
```

---

# 125. Sistema de unidades

Para mantener consistencia se recomienda:

```text
volumen
→ ml

tiempo corto
→ ms

timestamp
→ Unix seconds

temperatura
→ °C

conteos
→ integer
```

---

# 126. Nombres semánticos

UART puede utilizar:

```text
287
```

porque su posición define el significado.

MQTT debe utilizar:

```json
"volume_ml": 287
```

porque el backend debe ser autoexplicativo.

---

# 127. No enviar información redundante innecesaria

El `device_id` ya forma parte del topic:

```text
maim/v1/devices/MM_TEST_001/event
```

Por ello no es obligatorio repetirlo dentro de cada evento mientras el protocolo mantenga esta arquitectura.

---

# 128. Seguridad

Los parámetros recibidos por UART deben considerarse datos no confiables hasta ser validados.

Una trama malformada no debe:

* provocar overflow;
* bloquear tareas;
* corromper memoria;
* generar JSON inválido;
* reiniciar el ESP32.

---

# 129. Tamaños máximos

Los límites definidos por `uart_protocol` también aplican a eventos.

Los nombres y parámetros deben permanecer dentro de esos límites.

No aumentar buffers arbitrariamente sin analizar RAM.

---

# 130. EVENT y comandos

Los eventos no requieren:

```text
command_id
```

porque no son respuesta directa a un comando MQTT.

---

# 131. Excepción conceptual

Un comando podría provocar posteriormente un evento.

Ejemplo:

```text
MQTT:
START_CALIBRATION
```

ATmega:

```text
ACK
...
DONE
```

y además:

```text
EVENT,CALIBRATION_COMPLETED,...
```

Estos mensajes tienen funciones distintas.

---

# 132. DONE vs EVENT

`DONE` significa:

> **La transacción solicitada terminó.**

`EVENT` significa:

> **Ocurrió un acontecimiento relevante que puede formar parte del histórico.**

No deben confundirse.

---

# 133. Ejemplo

```text
CMD 25 START_CALIBRATION
```

respuesta:

```text
<ACK,25>
```

posteriormente:

```text
<EVENT,CALIBRATION_COMPLETED,468.2>
```

y:

```text
<DONE,25>
```

El orden exacto deberá definirse según cada comando.

---

# 134. EVENT y Transaction Manager

Los eventos no deben modificar directamente el estado de una transacción salvo que explícitamente se diseñe esa relación.

Actualmente:

```text
ACK / DONE / NACK
→ transaction_manager

EVENT
→ mqtt_manager
```

---

# 135. Estado actual del módulo de eventos

```text
[PROBADO] recepción EVENT por UART
[PROBADO] parsing de EVENT
[PROBADO] callback Device Manager
[PROBADO] publicación inmediata MQTT
[PROBADO] generación event_id
[PROBADO] timestamp Unix
[PROBADO] severity info
[PROBADO] objeto data
[PROBADO] DISPENSE_STARTED
[PROBADO] DISPENSE_COMPLETED
[PROBADO] CALIBRATION_COMPLETED
[PROBADO] PROGRAMMING_CHANGED

[POR DEFINIR] event_id persistente
[POR DEFINIR] catálogo completo de eventos
[POR DEFINIR] política de eventos de infraestructura
[PLANEADO] cola offline
[PLANEADO] deduplicación backend
[PLANEADO] eventos ERROR_RAISED / ERROR_CLEARED
[PLANEADO] almacenamiento histórico
```

---

# 136. Principios de diseño

1. **Un evento representa algo que ocurrió.**
2. Un evento no representa estado permanente.
3. Los eventos no se almacenan en `device_manager`.
4. Los eventos deben publicarse inmediatamente.
5. `/event` no debe utilizar retained.
6. Cada evento debe tener un tipo estable.
7. Los parámetros UART deben ser compactos.
8. MQTT debe utilizar nombres semánticos.
9. Cada dato debe tener unidades claramente definidas.
10. El ESP32 agrega timestamp y `event_id`.
11. El ATmega no necesita conocer MQTT ni JSON.
12. Eventos desconocidos no deben bloquear el sistema.
13. QoS 1 implica posibilidad de duplicados.
14. El backend deberá considerar deduplicación.
15. La pérdida de MQTT requerirá una futura cola de eventos.
16. Los eventos críticos podrán requerir persistencia.
17. Estado, error, evento y transacción son conceptos distintos.

---

# 137. Arquitectura final

```text
               CONTROLADOR
                    │
                    │
      <EVENT,TYPE,PARAMS...>
                    │
                    ▼
             uart_protocol
                    │
                    ▼
             device_manager
                    │
              event_callback
                    │
                    ▼
              mqtt_manager
                    │
          agrega metadata
                    │
        ┌───────────┼───────────┐
        │           │           │
     event_id       ts       severity
        │           │           │
        └───────────┼───────────┘
                    │
                    ▼
                  JSON
                    │
                    ▼
          MQTT QoS 1 / no retain
                    │
                    ▼
                Mosquitto
                    │
                    ▼
             Backend futuro
                    │
          ┌─────────┼─────────┐
          ▼         ▼         ▼
       Histórico  Analytics  Alertas
```

---

# 138. Resumen

El sistema de eventos proporciona la ruta para transportar acontecimientos puntuales desde el controlador principal hasta la infraestructura MAIM.

La filosofía puede resumirse como:

```text
Controlador
→ detecta qué ocurrió

UART Protocol
→ transporta el evento

Device Manager
→ valida y enruta

ESP32
→ agrega contexto de red

MQTT Manager
→ genera representación JSON

Mosquitto
→ transporta hacia servidor

Backend
→ almacena e interpreta
```

La implementación actual confirma que la ruta:

```text
EVENT UART
→ ESP32
→ MQTT
```

funciona correctamente y establece la base para construir posteriormente **históricos de operación, estadísticas de uso, mantenimiento, auditoría, diagnóstico y alertas**.

---

# 139. Relación con otros documentos

Arquitectura general:

```text
01_ARQUITECTURA_GENERAL.md
```

MQTT:

```text
08_PROTOCOLO_MQTT.md
```

UART:

```text
09_PROTOCOLO_UART_INTERNO.md
```

Device Manager:

```text
10_DEVICE_MANAGER.md
```

Comandos y transacciones:

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

**Fin del documento — `11_EVENTOS.md`**
