# MAIM Connectivity

## Device Manager

**Documento:** 10_DEVICE_MANAGER.md
**Proyecto:** MAIM Connectivity
**Módulo:** `device_manager.c / device_manager.h`
**Versión de arquitectura:** v0.1.0
**Estado:** [PROBADO] Interpretación semántica y almacenamiento interno funcionales

---

# 1. Propósito

`device_manager` es la capa encargada de interpretar semánticamente las tramas recibidas desde el controlador principal y mantener una representación interna del estado actual del equipo.

Su función principal es convertir mensajes del tipo:

```text
<STATE,MODE,STANDBY>
<METRIC,WATER_TOTAL_ML,16000>
<SENSOR,COLD_TANK_C,7.8>
<OUTPUT,VALVE,1>
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

en un estado interno organizado que posteriormente puede ser utilizado por:

```text
mqtt_manager
```

para construir mensajes como:

```text
state/reported
```

---

# 2. Posición dentro de la arquitectura

El flujo principal es:

```text
Controlador principal
        │
        │ UART
        ▼
   uart_protocol
        │
        │ uart_protocol_frame_t
        ▼
  device_manager
        │
        ├── STATE
        ├── METRIC
        ├── SENSOR
        ├── OUTPUT
        ├── ERROR
        ├── EVENT
        └── SNAPSHOT
        │
        ▼
   mqtt_manager
```

Para respuestas de transacciones:

```text
ACK / DONE / NACK
        │
        ▼
device_manager
        │
        ▼
transaction_manager
```

---

# 3. Separación de responsabilidades

La arquitectura divide claramente tres niveles:

```text
uart_protocol
→ sintaxis

device_manager
→ semántica y estado

mqtt_manager
→ representación MQTT / JSON
```

Ejemplo:

```text
<STATE,MODE,STANDBY>
```

`uart_protocol` determina:

```text
TYPE = STATE
FIELD_COUNT = 2
FIELD[0] = MODE
FIELD[1] = STANDBY
```

`device_manager` determina:

```text
STATE válido
KEY = MODE
VALUE = STANDBY
```

y lo almacena.

Posteriormente `mqtt_manager` puede convertirlo a:

```json
{
  "state": {
    "mode": "STANDBY"
  }
}
```

---

# 4. Objetivo arquitectónico

`device_manager` evita que `mqtt_manager` tenga que conocer directamente el formato UART.

Sin esta capa, MQTT tendría que procesar tramas como:

```text
<STATE,...>
<METRIC,...>
<SENSOR,...>
```

lo cual mezclaría responsabilidades.

La separación correcta es:

```text
UART
 ↓
Protocol Parser
 ↓
Device State
 ↓
MQTT Representation
```

---

# 5. Estado interno

El módulo mantiene actualmente varias tablas en RAM:

```text
states
metrics
sensors
outputs
errors
```

Además mantiene:

```text
controller_info
snapshot state
callbacks
```

---

# 6. Modelo KEY → VALUE

Las categorías:

```text
STATE
METRIC
SENSOR
OUTPUT
```

utilizan una estructura genérica:

```text
KEY → VALUE
```

Ejemplo:

```text
STATE

MODE → STANDBY
BUSY → 0
```

```text
METRIC

WATER_TOTAL_ML → 16000
DISPENSE_COUNT → 48
```

---

# 7. Estructura genérica

Conceptualmente:

```c
typedef struct
{
    bool used;

    char key[DEVICE_MANAGER_KEY_LEN];
    char value[DEVICE_MANAGER_VALUE_LEN];

} device_value_entry_t;
```

Cada entrada indica:

```text
used
→ slot ocupado

key
→ nombre del dato

value
→ valor actual
```

---

# 8. Ventaja del modelo genérico

No existe una estructura rígida como:

```c
struct {
    float cold_tank;
    float hot_tank;
    bool valve;
    bool compressor;
    ...
};
```

porque eso obligaría a modificar `device_manager` para cada modelo.

Con KEY → VALUE:

```text
MAIM MINI
→ pocas claves

MAIM ELITE
→ muchas claves
```

pueden utilizar el mismo motor.

---

# 9. Límites actuales

La implementación define límites internos para evitar uso dinámico descontrolado de memoria.

Actualmente:

```text
STATE       → 16 entradas
METRIC      → 24 entradas
SENSOR      → 32 entradas
OUTPUT      → 24 entradas
ERROR       → 16 entradas
```

Estos valores pueden cambiar posteriormente si un modelo lo requiere.

---

# 10. Tamaño de claves

Actualmente:

```text
DEVICE_MANAGER_KEY_LEN = 32
```

Ejemplos válidos:

```text
MODE
WATER_TOTAL_ML
FLOW_SENSOR_FAIL
COLD_TANK_C
```

---

# 11. Tamaño de valores

Actualmente:

```text
DEVICE_MANAGER_VALUE_LEN = 40
```

Esto permite almacenar valores como texto.

---

# 12. Por qué los valores se guardan como texto

El protocolo UART transporta caracteres ASCII.

Ejemplo:

```text
16000
7.8
1
STANDBY
```

Guardar inicialmente el valor como texto permite que el sistema acepte nuevas claves sin conocer previamente su tipo.

---

# 13. Ejemplo

Entrada:

```text
<METRIC,WATER_TOTAL_ML,16000>
```

Se almacena:

```text
KEY:
WATER_TOTAL_ML

VALUE:
"16000"
```

Cuando MQTT necesita utilizarlo como número:

```text
"16000"
   ↓
get_metric_int64()
   ↓
16000
```

---

# 14. Actualización de entradas

Cuando se recibe por primera vez:

```text
<METRIC,WATER_TOTAL_ML,15420>
```

se crea una entrada.

Si posteriormente llega:

```text
<METRIC,WATER_TOTAL_ML,16000>
```

no se crea otra.

Se actualiza la entrada existente.

Resultado:

```text
WATER_TOTAL_ML = 16000
```

---

# 15. Prueba realizada

Durante validación se enviaron:

```text
<METRIC,WATER_TOTAL_ML,15420>
```

y después:

```text
<METRIC,WATER_TOTAL_ML,16000>
```

El estado final mostró únicamente:

```text
WATER_TOTAL_ML = 16000
```

Estado:

```text
[PROBADO]
```

---

# 16. Inicialización

El módulo se inicializa mediante:

```c
device_manager_init();
```

La función limpia:

```text
states
metrics
sensors
outputs
errors
controller_info
snapshot
```

dejando el sistema en estado conocido.

---

# 17. Orden de inicialización

La secuencia utilizada actualmente es:

```text
uart_protocol_init()
        ↓
device_manager_init()
        ↓
transaction_manager_init()
        ↓
registrar callbacks
```

---

# 18. Procesamiento principal

La función principal es:

```c
device_manager_process_frame(
    const uart_protocol_frame_t *frame
);
```

Recibe una trama ya parseada y la procesa según:

```text
frame->type
```

---

# 19. Procesamiento de STATE

Formato esperado:

```text
<STATE,KEY,VALUE>
```

Debe contener exactamente:

```text
2 campos
```

Ejemplo:

```text
<STATE,MODE,STANDBY>
```

Resultado:

```text
STATE actualizado | MODE=STANDBY
```

---

# 20. STATE inválido

Ejemplo:

```text
<STATE,MODE>
```

El parser puede aceptarlo sintácticamente, pero `device_manager` responde:

```text
STATE invalido. Se esperaban KEY,VALUE
```

y devuelve:

```text
ESP_ERR_INVALID_ARG
```

---

# 21. Procesamiento de METRIC

Formato:

```text
<METRIC,KEY,VALUE>
```

Ejemplo:

```text
<METRIC,WATER_TOTAL_ML,16000>
```

Resultado:

```text
METRIC actualizado | WATER_TOTAL_ML=16000
```

---

# 22. Procesamiento de SENSOR

Formato:

```text
<SENSOR,KEY,VALUE>
```

Ejemplo:

```text
<SENSOR,COLD_TANK_C,7.8>
```

Resultado interno:

```text
COLD_TANK_C = 7.8
```

---

# 23. Procesamiento de OUTPUT

Formato:

```text
<OUTPUT,KEY,VALUE>
```

Ejemplo:

```text
<OUTPUT,VALVE,1>
```

Resultado:

```text
VALVE = 1
```

---

# 24. Procesamiento de ERROR

Formato:

```text
<ERROR,CODE,SEVERITY>
```

Ejemplo:

```text
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

El error se agrega a la tabla de errores activos.

---

# 25. Estructura de error

Conceptualmente:

```c
typedef struct
{
    bool used;

    char code[DEVICE_MANAGER_KEY_LEN];
    char severity[16];

} device_error_entry_t;
```

---

# 26. Severidades válidas

Actualmente:

```text
INFO
WARNING
ERROR
CRITICAL
```

---

# 27. Error con severidad inválida

Ejemplo:

```text
<ERROR,FLOW_SENSOR_FAIL,LOQUESEA>
```

debe ser rechazado.

La severidad no forma parte de un campo libre.

---

# 28. Error repetido

Si un error ya está activo:

```text
FLOW_SENSOR_FAIL
```

y vuelve a recibirse, no se crea necesariamente una segunda entrada.

La severidad puede actualizarse.

---

# 29. ERROR_CLEAR

Formato:

```text
<ERROR_CLEAR,CODE>
```

Ejemplo:

```text
<ERROR_CLEAR,FLOW_SENSOR_FAIL>
```

El módulo busca el error activo y libera la entrada.

---

# 30. ERROR_CLEAR de error inexistente

Si se recibe:

```text
<ERROR_CLEAR,FLOW_SENSOR_FAIL>
```

cuando el error ya no está activo, el módulo lo registra como advertencia pero mantiene el estado final correcto:

```text
error no activo
```

Por ello no se considera un fallo grave.

---

# 31. Contador de errores

La API permite:

```c
device_manager_get_error_count();
```

Ejemplo:

```text
0
```

o:

```text
3
```

según los errores activos.

---

# 32. Obtener errores

La función:

```c
device_manager_get_error(index);
```

permite recorrer los errores activos.

Esto es utilizado por `mqtt_manager`.

---

# 33. Construcción MQTT de errores

Ejemplo interno:

```text
FLOW_SENSOR_FAIL
ERROR
```

se convierte a:

```json
{
  "code": "FLOW_SENSOR_FAIL",
  "severity": "ERROR"
}
```

---

# 34. Problema encontrado: doble campo `errors`

Durante la primera integración MQTT se generó:

```json
"errors":[...],
...
"errors":[]
```

porque `mqtt_manager` creaba dos veces el mismo elemento JSON.

El problema no estaba en `device_manager`.

La solución fue mantener un único bloque `errors` y agregar dentro de él las entradas obtenidas mediante:

```text
device_manager_get_error_count()
device_manager_get_error()
```

---

# 35. Problema encontrado: error creado pero no agregado

En una primera versión se construía:

```c
cJSON *error_json =
    cJSON_CreateObject();
```

pero faltaba:

```c
cJSON_AddItemToArray(
    errors,
    error_json
);
```

por lo que el error no quedaba realmente dentro del array.

Esto se corrigió.

---

# 36. Controller Info

`device_manager` mantiene información del controlador principal.

Estructura conceptual:

```c
typedef struct
{
    bool valid;

    char mcu[];
    char model[];
    char hw_rev[];
    char fw_version[];

} device_controller_info_t;
```

---

# 37. HELLO

La información se obtiene mediante:

```text
<HELLO,MCU,MODEL,HW_REV,FW_VERSION>
```

Ejemplo:

```text
<HELLO,ATMEGA328P,MAIM_MINI,1.0,1.4.0>
```

---

# 38. Resultado HELLO

```text
MCU:
ATMEGA328P

MODEL:
MAIM_MINI

HW:
1.0

FW:
1.4.0
```

y:

```text
controller_info.valid = true
```

---

# 39. Uso futuro de Controller Info

Permitirá:

* comprobar compatibilidad;
* mostrar firmware del controlador;
* decidir comandos soportados;
* validar actualización AVR;
* reportar versión al backend.

---

# 40. EVENT

`device_manager` no almacena los eventos como estado permanente.

Cuando recibe:

```text
<EVENT,...>
```

invoca un callback.

---

# 41. Event Callback

Tipo conceptual:

```c
typedef void (*device_manager_event_callback_t)(
    const char *event_type,
    const uart_protocol_frame_t *frame
);
```

---

# 42. Ruta de evento

```text
uart_protocol
      ↓
device_manager
      ↓
event_callback
      ↓
mqtt_manager_publish_event()
```

---

# 43. Razón para no almacenar EVENT como estado

Un evento representa:

```text
algo ocurrió
```

no:

```text
condición actual
```

Ejemplo:

```text
DISPENSE_COMPLETED
```

no tiene sentido como valor permanente dentro del Device Manager.

---

# 44. ACK / DONE / NACK

Las respuestas de transacción tampoco se almacenan como estado.

Cuando llegan:

```text
<ACK,15>
<DONE,15>
<NACK,15,BUSY>
```

`device_manager` las dirige mediante callback a:

```text
transaction_manager
```

---

# 45. Transaction Callback

Conceptualmente:

```c
typedef void (*device_manager_transaction_callback_t)(
    uart_frame_type_t type,
    uint16_t transaction_id,
    const uart_protocol_frame_t *frame
);
```

---

# 46. Ruta ACK

```text
<ACK,15>
   ↓
uart_protocol
   ↓
device_manager
   ↓
transaction callback
   ↓
transaction_manager
```

---

# 47. Parseo de Transaction ID

`device_manager` convierte el ID recibido como texto:

```text
"15"
```

a:

```text
uint16_t 15
```

mediante validación numérica.

---

# 48. Transaction ID inválido

Ejemplo:

```text
<ACK,ABC>
```

debe rechazarse.

También se rechazan valores mayores a:

```text
65535
```

---

# 49. SNAPSHOT

El módulo mantiene:

```text
snapshot_active
snapshot_transaction_id
```

---

# 50. SNAPSHOT BEGIN

Entrada:

```text
<SNAPSHOT,BEGIN,200>
```

Resultado:

```text
snapshot_active = true
snapshot_transaction_id = 200
```

---

# 51. SNAPSHOT END

Entrada:

```text
<SNAPSHOT,END,200>
```

Se valida que:

```text
snapshot_active == true
```

y:

```text
ID recibido == snapshot_transaction_id
```

---

# 52. Snapshot incorrecto

Ejemplo:

```text
<SNAPSHOT,BEGIN,200>
...
<SNAPSHOT,END,201>
```

debe ser rechazado.

Resultado:

```text
ESP_ERR_INVALID_STATE
```

---

# 53. Uso futuro del Snapshot

Permitirá sincronizar un conjunto coherente de variables.

Ejemplo:

```text
<SNAPSHOT,BEGIN,200>
<STATE,MODE,STANDBY>
<METRIC,WATER_TOTAL_ML,16000>
<OUTPUT,VALVE,0>
<SNAPSHOT,END,200>
```

Después puede publicarse:

```text
state/reported
```

una sola vez.

---

# 54. Snapshot todavía no reemplaza cache

Actualmente las entradas siguen almacenándose directamente en las tablas.

El snapshot se utiliza como indicador de delimitación.

Una futura evolución puede utilizar staging temporal si se requiere atomicidad estricta.

---

# 55. CONFIG

Actualmente se reconoce:

```text
<CONFIG,KEY,VALUE>
```

y se valida que tenga dos campos.

La persistencia completa de configuración todavía no está implementada.

Estado:

```text
[EN DESARROLLO / PLANEADO]
```

---

# 56. CMD recibido desde controlador

Por arquitectura normal:

```text
CMD
```

debe viajar:

```text
ESP32 → controlador
```

Por ello, si `device_manager` recibe una trama:

```text
<CMD,...>
```

desde el controlador, se considera inesperada.

Actualmente:

```text
CMD recibido desde controlador. Ignorado
```

y devuelve:

```text
ESP_ERR_NOT_SUPPORTED
```

---

# 57. Getters de texto

La API expone:

```c
device_manager_get_state(key);
```

```c
device_manager_get_metric(key);
```

```c
device_manager_get_sensor(key);
```

```c
device_manager_get_output(key);
```

---

# 58. Ejemplo getter STATE

```c
const char *mode =
    device_manager_get_state("MODE");
```

Resultado posible:

```text
"DISPENSING"
```

---

# 59. Clave inexistente

Si no existe:

```text
MODE
```

la función devuelve:

```text
NULL
```

Esto permite omitir campos desconocidos en MQTT.

---

# 60. Getters tipados

Actualmente existen funciones para convertir valores.

Ejemplos:

```c
device_manager_get_metric_int64()
```

```c
device_manager_get_sensor_float()
```

```c
device_manager_get_output_bool()
```

---

# 61. Metric → int64

Ejemplo:

```text
WATER_TOTAL_ML = "16000"
```

se convierte a:

```text
16000
```

como:

```text
int64_t
```

---

# 62. Sensor → float

Ejemplo:

```text
COLD_TANK_C = "7.8"
```

se convierte a:

```text
7.8
```

---

# 63. Output → bool

Valores permitidos actualmente:

```text
"1"
"0"
```

Conversión:

```text
1 → true
0 → false
```

---

# 64. Booleano inválido

Ejemplo:

```text
<OUTPUT,VALVE,YES>
```

puede almacenarse como texto porque la trama es genérica, pero:

```c
device_manager_get_output_bool()
```

devolverá:

```text
ESP_ERR_INVALID_ARG
```

---

# 65. Separación entre almacenamiento y tipado

Esta decisión es intencional.

```text
almacenamiento
→ flexible

consumo
→ tipado
```

Así se evita que el núcleo de `device_manager` dependa de todas las posibles claves.

---

# 66. Integración con `mqtt_manager`

`mqtt_manager_publish_state()` consulta directamente el estado interno.

Ejemplo:

```c
const char *mode =
    device_manager_get_state("MODE");
```

---

# 67. BUSY

Actualmente:

```text
BUSY = "1"
```

se convierte en JSON:

```json
"busy": true
```

---

# 68. WATER_TOTAL_ML

```c
device_manager_get_metric_int64(
    "WATER_TOTAL_ML",
    &water_total_ml
);
```

genera:

```json
"water_total_ml": 16000
```

---

# 69. LAST_DISPENSE_ML

```text
LAST_DISPENSE_ML = 287
```

genera:

```json
"last_dispense_ml": 287
```

---

# 70. DISPENSE_COUNT

```text
DISPENSE_COUNT = 48
```

genera:

```json
"dispense_count": 48
```

---

# 71. VALVE

```text
VALVE = 1
```

genera:

```json
"valve": true
```

---

# 72. Errores MQTT

Los errores activos se recorren:

```text
for each error
```

y se agregan a:

```json
"errors": []
```

---

# 73. Prueba completa validada

Se enviaron manualmente:

```text
<STATE,MODE,DISPENSING>
<STATE,BUSY,1>
<METRIC,WATER_TOTAL_ML,16000>
<METRIC,LAST_DISPENSE_ML,287>
<METRIC,DISPENSE_COUNT,48>
<OUTPUT,VALVE,1>
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

Después se solicitó:

```text
GET_STATE
```

---

# 74. Resultado MQTT validado

El broker recibió:

```json
{
  "state": {
    "mode": "DISPENSING",
    "busy": true
  },
  "metrics": {
    "water_total_ml": 16000,
    "last_dispense_ml": 287,
    "dispense_count": 48
  },
  "outputs": {
    "valve": true
  },
  "errors": [
    {
      "code": "FLOW_SENSOR_FAIL",
      "severity": "ERROR"
    }
  ]
}
```

Esto confirmó que:

```text
UART
→ Device Manager
→ MQTT
```

funciona correctamente.

---

# 75. Print de depuración

Existe:

```c
device_manager_print_status();
```

que muestra el contenido de las tablas.

---

# 76. Ejemplo de salida

```text
DEVICE MANAGER STATUS

STATE
MODE = STANDBY
BUSY = 0

METRICS
WATER_TOTAL_ML = 16000

SENSORS
COLD_TANK_C = 7.8

OUTPUTS
VALVE = 0

ERRORS
(sin errores)
```

---

# 77. Uso de `print_status`

Es útil durante:

* desarrollo;
* pruebas UART;
* diagnóstico;
* validación del parser.

No debe ejecutarse innecesariamente en producción si genera demasiado log.

---

# 78. Problema potencial: tabla llena

Si una tabla alcanza su máximo:

```text
STATE = 16
METRIC = 24
...
```

y llega una nueva clave, `set_value()` devuelve:

```text
ESP_ERR_NO_MEM
```

---

# 79. Qué significa `ESP_ERR_NO_MEM` aquí

No necesariamente significa que todo el heap del ESP32 se agotó.

Puede significar:

> La tabla fija de esa categoría ya no tiene slots disponibles.

---

# 80. Ventaja de tablas fijas

Evitan:

* fragmentación dinámica;
* crecimiento ilimitado;
* comportamiento impredecible.

Son apropiadas para firmware embebido.

---

# 81. Ajuste de límites

Si un producto realmente necesita:

```text
40 sensores
```

puede aumentarse:

```text
DEVICE_MANAGER_MAX_SENSORS
```

después de evaluar:

* RAM;
* tamaño de claves;
* arquitectura del modelo.

---

# 82. No aumentar límites sin necesidad

Reservar cientos de entradas innecesarias consumiría RAM permanentemente.

Los límites deben dimensionarse según familias reales de producto.

---

# 83. Concurrencia

Actualmente `device_manager` es utilizado principalmente dentro del flujo de recepción y consulta del ESP32.

Si en el futuro múltiples tareas escriben y leen simultáneamente con mayor complejidad, deberá evaluarse protección mediante:

```text
mutex
critical section
```

Estado actual:

```text
[NO REQUERIDO EN LAS PRUEBAS ACTUALES]
```

---

# 84. Riesgo futuro de concurrencia

Ejemplo:

```text
UART task
→ actualiza tabla

MQTT task
→ construye JSON simultáneamente
```

Si las operaciones se vuelven más complejas, podría producirse una lectura inconsistente.

Debe revisarse cuando se implemente `uart_manager` físico.

---

# 85. Snapshot como ayuda de coherencia

El mecanismo snapshot también puede ayudar a evitar publicar estados parciales durante una sincronización completa.

---

# 86. Persistencia

Actualmente el estado almacenado por `device_manager` vive en:

```text
RAM
```

Por tanto:

```text
reinicio ESP32
→ Device Manager vacío
```

---

# 87. Esto es intencional

No es necesario persistir continuamente:

```text
STATE
SENSOR
OUTPUT
```

en Flash porque el controlador principal es la fuente autoritativa.

Después de reiniciar, el ESP32 debe volver a consultar al controlador.

---

# 88. Futuro GET_STATE automático

La arquitectura prevista es:

```text
ESP32 boot
   ↓
UART disponible
   ↓
HELLO
   ↓
GET_STATE
   ↓
SNAPSHOT
   ↓
Device Manager reconstruido
```

Estado:

```text
[PLANEADO]
```

---

# 89. Fuente de verdad

Para estado físico:

```text
Controlador principal
```

es la fuente de verdad.

`device_manager` mantiene una copia local.

MQTT publica esa copia.

---

# 90. No inventar estado

Si el controlador no ha enviado:

```text
COLD_TANK_C
```

el ESP32 no debe inventar:

```text
0
```

como temperatura.

Debe:

```text
omitir el campo
```

o marcarlo explícitamente si en el futuro se define una semántica de desconocido.

---

# 91. Estado desconocido

Por eso los getters devuelven:

```text
NULL
ESP_ERR_NOT_FOUND
```

cuando un valor todavía no existe.

---

# 92. Ventaja para múltiples modelos

Un MAIM MINI puede no tener:

```text
HOT_TANK_C
```

mientras MAIM ELITE sí.

La ausencia de la clave no es necesariamente un error.

---

# 93. Catálogo de capabilities futuro

En el futuro el backend podrá conocer qué campos esperar mediante:

```text
capabilities
model
hardware revision
```

Estado:

```text
[PLANEADO]
```

---

# 94. Device Manager no debe conocer broker

Regla:

> `device_manager` no debe contener IP, usuario MQTT, topics ni lógica del broker.

Su responsabilidad termina en:

```text
estado + callbacks
```

---

# 95. Device Manager no debe controlar hardware

Tampoco debe realizar directamente:

```text
gpio_set_level()
```

para válvulas o bombas.

Ese control pertenece al controlador físico.

---

# 96. Device Manager no debe parsear UART crudo

No debe recibir bytes individuales.

Eso pertenece a:

```text
uart_protocol
```

y en el futuro:

```text
uart_manager
```

---

# 97. Arquitectura limpia

```text
uart_manager
→ hardware

uart_protocol
→ gramática

device_manager
→ significado/estado

mqtt_manager
→ red
```

---

# 98. Flujo EVENT especial

Los eventos atraviesan `device_manager`, pero no se guardan como tabla.

```text
EVENT
 ↓
validación
 ↓
callback
```

---

# 99. Flujo TRANSACTION especial

```text
ACK/DONE/NACK
 ↓
validación de ID
 ↓
callback
 ↓
transaction_manager
```

---

# 100. Flujo STATE normal

```text
STATE/METRIC/SENSOR/OUTPUT/ERROR
 ↓
validación
 ↓
almacenamiento
```

---

# 101. Tratamiento de errores de procesamiento

Si `device_manager_process_frame()` devuelve error:

```text
ESP_ERR_INVALID_ARG
ESP_ERR_NOT_SUPPORTED
ESP_ERR_NO_MEM
ESP_ERR_INVALID_STATE
```

`main.c` registra:

```text
Device Manager rechazo trama ...
```

---

# 102. No bloquear el parser

Una trama semánticamente inválida no debe detener las siguientes.

Ejemplo:

```text
<STATE,MODE>
```

rechazada.

Después:

```text
<STATE,MODE,STANDBY>
```

debe procesarse normalmente.

---

# 103. Validación realizada

Durante pruebas se enviaron deliberadamente tramas incorrectas.

Se verificó que:

```text
se rechazaban
```

y que posteriormente las tramas correctas seguían procesándose.

Estado:

```text
[PROBADO]
```

---

# 104. Actualización de error

Un código ya existente puede recibir una nueva severidad.

Ejemplo:

```text
<ERROR,TEMP_HIGH,WARNING>
```

posteriormente:

```text
<ERROR,TEMP_HIGH,CRITICAL>
```

La entrada puede actualizarse.

---

# 105. Orden de errores

La tabla interna no debe interpretarse como un ranking de prioridad.

La severidad proporciona el significado.

---

# 106. Eliminación de errores

Cuando se borra una entrada mediante `memset`, queda un slot libre que puede ser reutilizado por otro error.

---

# 107. Error Count

La función cuenta únicamente:

```text
errors[i].used == true
```

por lo que los huecos internos no afectan la iteración pública.

---

# 108. Obtener error por índice lógico

`device_manager_get_error(0)` devuelve el primer error activo, no necesariamente el slot físico `errors[0]`.

Esto abstrae la estructura interna.

---

# 109. Getter Controller Info

La API:

```c
device_manager_get_controller_info();
```

devuelve un puntero a la estructura actual.

Antes de HELLO:

```text
valid = false
```

---

# 110. Snapshot getters

La API incluye:

```c
device_manager_snapshot_active();
```

y:

```c
device_manager_snapshot_id();
```

---

# 111. Uso futuro de snapshot_active

Permitirá decidir:

```text
si snapshot activo
→ no publicar state/reported todavía

cuando END
→ publicar estado consolidado
```

Estado:

```text
[PLANEADO]
```

---

# 112. Conversión numérica segura

Los getters tipados utilizan:

```text
strtoll
strtof
```

y verifican:

```text
endptr
```

para asegurar que todo el string sea numérico.

---

# 113. Ejemplo inválido

```text
"123abc"
```

no se acepta como:

```text
123
```

porque quedarían caracteres sin consumir.

Esto evita conversiones silenciosas incorrectas.

---

# 114. Conversiones futuras

Podrían añadirse:

```text
get_state_bool()
get_metric_double()
get_sensor_int()
get_config_float()
```

según necesidades.

---

# 115. No sobrediseñar getters

No deben añadirse decenas de getters específicos por clave.

Preferir conversiones genéricas por tipo.

---

# 116. MQTT actual no exporta todas las tablas automáticamente

Aunque `device_manager` puede almacenar múltiples claves, `mqtt_manager_publish_state()` actualmente agrega explícitamente ciertas claves.

Ejemplo:

```text
MODE
BUSY
WATER_TOTAL_ML
LAST_DISPENSE_ML
DISPENSE_COUNT
VALVE
```

---

# 117. Motivo

Esto mantiene control sobre:

* nombres JSON;
* tipos;
* unidades;
* schema MQTT.

No se desea convertir ciegamente cualquier KEY UART en un campo MQTT arbitrario.

---

# 118. Evolución posible

En el futuro puede existir una capa de:

```text
capability/schema mapping
```

que defina de manera declarativa:

```text
UART KEY
→ MQTT path
→ type
```

Estado:

```text
[PLANEADO]
```

---

# 119. Ejemplo de mapping futuro

Conceptualmente:

```text
WATER_TOTAL_ML
→ metrics.water_total_ml
→ int64
```

```text
COLD_TANK_C
→ sensors.cold_tank_c
→ double
```

---

# 120. Esto permitiría firmware más genérico

Un mismo `mqtt_manager` podría soportar múltiples modelos con tablas de capabilities sin incorporar cada clave manualmente.

No implementado todavía.

---

# 121. Relación con memoria Flash

`device_manager` no utiliza NVS actualmente para estado operativo.

Sus tablas viven en RAM.

Esto evita escrituras Flash frecuentes.

---

# 122. Ventaja para sensores

Un sensor puede cambiar cientos o miles de veces sin desgastar NVS.

---

# 123. Métricas persistentes

Si una métrica necesita persistencia:

```text
WATER_TOTAL_ML
```

la fuente autoritativa debe seguir siendo el controlador o una estrategia de persistencia explícita.

No debe asumirse que `device_manager` persiste datos después de reboot.

---

# 124. Debug temporal

Durante pruebas se llamó:

```c
device_manager_print_status();
```

después de cada trama.

Esto permitió observar cómo las tablas pasaban:

```text
vacío
→ valor recibido
→ valor actualizado
```

---

# 125. Resultado de las pruebas

Las pruebas confirmaron:

```text
[PROBADO] inserción
[PROBADO] actualización
[PROBADO] consulta
[PROBADO] errores
[PROBADO] error clear
[PROBADO] validación semántica
[PROBADO] callbacks
[PROBADO] integración MQTT
```

---

# 126. Posibles problemas futuros

Debe vigilarse:

```text
tabla llena
concurrencia
claves demasiado largas
valores demasiado largos
tipos incorrectos
snapshot incompleto
controlador reiniciado
UART perdido
```

---

# 127. Reinicio del controlador

Si el ATmega reinicia mientras ESP32 continúa, el cache puede quedar temporalmente desactualizado.

Una futura estrategia debe detectar:

```text
HELLO nuevo
```

y reconstruir el estado.

---

# 128. Estrategia futura ante HELLO nuevo

Conceptualmente:

```text
HELLO recibido
   ↓
detectar reinicio controlador
   ↓
limpiar estado dinámico
   ↓
GET_STATE
   ↓
SNAPSHOT
```

Estado:

```text
[PLANEADO]
```

---

# 129. Watchdog lógico futuro

La ausencia de mensajes del controlador durante un periodo determinado podría generar:

```text
CONTROLLER_OFFLINE
```

como estado de diagnóstico.

Todavía no está implementado.

---

# 130. Device Manager y watchdog no son lo mismo

`device_manager` representa datos.

Un futuro:

```text
controller_monitor
```

o mecanismo equivalente puede supervisar salud del enlace.

---

# 131. Tabla resumen

| Entrada UART  | Acción Device Manager             |
| ------------- | --------------------------------- |
| `HELLO`       | Actualiza información controlador |
| `STATE`       | Guarda KEY/VALUE                  |
| `METRIC`      | Guarda KEY/VALUE                  |
| `SENSOR`      | Guarda KEY/VALUE                  |
| `OUTPUT`      | Guarda KEY/VALUE                  |
| `ERROR`       | Agrega/actualiza error            |
| `ERROR_CLEAR` | Elimina error                     |
| `EVENT`       | Callback inmediato                |
| `SNAPSHOT`    | Control BEGIN/END                 |
| `ACK`         | Callback transacción              |
| `DONE`        | Callback transacción              |
| `NACK`        | Callback transacción              |
| `CONFIG`      | Valida / soporte parcial          |
| `CMD`         | Rechazado desde controlador       |

---

# 132. API principal

```text
device_manager_init()

device_manager_process_frame()

device_manager_set_event_callback()

device_manager_set_transaction_callback()

device_manager_get_state()

device_manager_get_metric()

device_manager_get_sensor()

device_manager_get_output()

device_manager_get_metric_int64()

device_manager_get_sensor_float()

device_manager_get_output_bool()

device_manager_get_error_count()

device_manager_get_error()

device_manager_get_controller_info()

device_manager_snapshot_active()

device_manager_snapshot_id()

device_manager_print_status()
```

---

# 133. Flujo completo de ejemplo

Entrada:

```text
<STATE,MODE,DISPENSING>
```

Proceso:

```text
uart_protocol
→ TYPE STATE
→ fields MODE / DISPENSING
```

```text
device_manager
→ valida 2 campos
→ busca MODE
→ crea/actualiza
```

Estado interno:

```text
MODE = DISPENSING
```

MQTT:

```json
{
  "state": {
    "mode": "DISPENSING"
  }
}
```

---

# 134. Flujo de error

Entrada:

```text
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

Device Manager:

```text
errors[]
→ FLOW_SENSOR_FAIL
→ ERROR
```

MQTT:

```json
{
  "errors": [
    {
      "code": "FLOW_SENSOR_FAIL",
      "severity": "ERROR"
    }
  ]
}
```

---

# 135. Flujo EVENT

Entrada:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

Device Manager:

```text
no almacenar
↓
event callback
```

MQTT:

```text
event
```

inmediato.

---

# 136. Flujo ACK

Entrada:

```text
<ACK,15>
```

Device Manager:

```text
validar ID
↓
transaction callback
```

Transaction Manager:

```text
ID 15
→ MQTT command_id
```

---

# 137. Principios de diseño

1. No mezclar parsing UART con semántica.
2. No mezclar Device Manager con MQTT.
3. No controlar hardware desde Device Manager.
4. Mantener estado en RAM.
5. El controlador es fuente de verdad.
6. Las claves desconocidas pueden coexistir.
7. Los valores se almacenan genéricamente.
8. Las capas consumidoras realizan conversión tipada.
9. Los eventos no son estado.
10. ACK/DONE/NACK no son estado.
11. Los errores activos requieren manejo explícito.
12. Los snapshots deben mantener coherencia.
13. Las tablas deben permanecer acotadas.
14. Una trama inválida nunca debe bloquear el sistema.
15. La arquitectura debe funcionar con distintos modelos.

---

# 138. Estado actual

```text
[PROBADO] inicialización
[PROBADO] STATE
[PROBADO] METRIC
[PROBADO] SENSOR
[PROBADO] OUTPUT
[PROBADO] ERROR
[PROBADO] ERROR_CLEAR
[PROBADO] HELLO parsing
[PROBADO] EVENT callback
[PROBADO] ACK callback
[PROBADO] DONE callback
[PROBADO] NACK callback
[PROBADO] SNAPSHOT BEGIN/END
[PROBADO] getters
[PROBADO] conversión int64
[PROBADO] conversión float
[PROBADO] conversión bool
[PROBADO] actualización de valores
[PROBADO] state/reported desde datos reales

[EN DESARROLLO] CONFIG completo

[PLANEADO] sincronización automática al boot
[PLANEADO] limpieza tras reboot del controlador
[PLANEADO] protección de concurrencia si se requiere
[PLANEADO] capabilities/schema mapping
[PLANEADO] controller health monitoring
```

---

# 139. Relación con otros documentos

Protocolo UART:

```text
09_PROTOCOLO_UART_INTERNO.md
```

MQTT:

```text
08_PROTOCOLO_MQTT.md
```

Eventos:

```text
11_EVENTOS.md
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

# 140. Resumen

`device_manager` constituye la memoria semántica temporal del equipo dentro del ESP32.

Su función puede resumirse como:

```text
UART Frame
    ↓
validación semántica
    ↓
estado interno
    ↓
MQTT / callbacks
```

Mantiene una copia flexible y desacoplada del estado reportado por el controlador principal y permite que el resto de MAIM Connectivity trabaje con datos normalizados sin depender directamente de la gramática UART.

Esta capa es la base de la integración actual entre **MAIM Internal UART Protocol v1** y **MAIM MQTT Protocol v1**.
