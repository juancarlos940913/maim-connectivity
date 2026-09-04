# MAIM Connectivity

## Protocolo UART Interno v1

**Documento:** 09_PROTOCOLO_UART_INTERNO.md
**Proyecto:** MAIM Connectivity
**Protocolo:** MAIM Internal UART Protocol v1
**Transporte actual de prueba:** Consola UART / monitor ESP-IDF
**Transporte previsto:** UART físico ESP32 ↔ controlador principal
**Formato:** ASCII delimitado
**Estado:** [PROBADO] Parser, Device Manager, eventos y transacciones funcionales

---

# 1. Propósito

Este documento define formalmente el protocolo de comunicación interna entre el **ESP32** y el **controlador principal** de un equipo MAIM.

Actualmente el controlador principal previsto es un:

```text
ATmega
```

pero el protocolo fue diseñado para no depender de una familia específica de microcontroladores.

La arquitectura general es:

```text
Servidor
   │
   │ MQTT
   ▼
ESP32
   │
   │ MAIM Internal UART Protocol v1
   ▼
Controlador principal
   │
   ├── sensores
   ├── actuadores
   ├── interfaz
   └── lógica física
```

---

# 2. Objetivo del protocolo

El protocolo debe permitir transportar de forma simple y extensible:

* estados;
* métricas;
* sensores;
* outputs;
* eventos;
* errores;
* configuración;
* comandos;
* respuestas;
* snapshots;
* identificación del controlador.

La intención es que el mismo protocolo pueda utilizarse en equipos con diferentes cantidades de funciones y sensores sin cambiar la estructura base.

---

# 3. Principios de diseño

MAIM Internal UART Protocol v1 se diseñó para ser:

```text
simple
legible
manual-testable
extensible
determinista
ligero
independiente del modelo
```

Durante desarrollo debe ser posible escribir una trama manualmente desde una terminal y comprender inmediatamente su contenido.

---

# 4. Formato general

Toda trama utiliza:

```text
<TYPE,FIELD1,FIELD2,...>
```

Ejemplo:

```text
<STATE,MODE,STANDBY>
```

---

# 5. Delimitadores

Inicio de trama:

```text
<
```

Fin de trama:

```text
>
```

Separador:

```text
,
```

Fin físico recomendado:

```text
\n
```

Ejemplo completo sobre UART:

```text
<STATE,MODE,STANDBY>\n
```

---

# 6. Codificación

La versión actual utiliza:

```text
ASCII
```

Los nombres de comandos y claves utilizan caracteres simples ASCII.

No utilizar acentos ni caracteres especiales dentro de las claves del protocolo.

---

# 7. Convención de nombres

Los identificadores del protocolo utilizan:

```text
UPPER_SNAKE_CASE
```

Ejemplos:

```text
STATE
WATER_TOTAL_ML
DISPENSE_COMPLETED
FLOW_SENSOR_FAIL
EXECUTION_TIMEOUT
```

---

# 8. Decimales

Utilizar:

```text
.
```

como separador decimal.

Correcto:

```text
7.8
```

Incorrecto:

```text
7,8
```

porque la coma ya es el separador de campos.

---

# 9. Booleanos

Dentro de UART utilizar:

```text
0
1
```

Interpretación:

```text
0 = false
1 = true
```

Ejemplo:

```text
<STATE,BUSY,1>
```

---

# 10. Unidades

Siempre que sea posible, la unidad debe formar parte de la clave.

Ejemplos:

```text
WATER_TOTAL_ML
COLD_TANK_C
RSSI_DBM
DISPENSE_TIME_MS
RUNTIME_SEC
```

Esto evita que el receptor tenga que inferir unidades.

---

# 11. Longitud máxima

La implementación actual utiliza:

```text
UART_PROTOCOL_MAX_FRAME_LEN = 128
```

Por lo tanto, una trama v1 no debe superar:

```text
127 caracteres útiles + terminación interna
```

El límite exacto debe respetar la implementación de buffer.

---

# 12. Número máximo de campos

La implementación actual utiliza:

```text
UART_PROTOCOL_MAX_FIELDS = 8
```

Esto significa que después del tipo pueden existir hasta 8 campos.

Ejemplo:

```text
<TYPE,F1,F2,F3,F4,F5,F6,F7,F8>
```

---

# 13. Longitud máxima de campo

La implementación actual utiliza:

```text
UART_PROTOCOL_MAX_FIELD_LEN = 32
```

Las claves y valores individuales no deben superar este límite.

---

# 14. Clases oficiales de trama

Actualmente existen:

```text
HELLO
CMD
ACK
DONE
NACK
STATE
METRIC
SENSOR
OUTPUT
EVENT
ERROR
ERROR_CLEAR
CONFIG
SNAPSHOT
```

---

# 15. Dirección general

| Clase         | Dirección típica        |
| ------------- | ----------------------- |
| `HELLO`       | Controlador → ESP32     |
| `CMD`         | ESP32 → Controlador     |
| `ACK`         | Controlador → ESP32     |
| `DONE`        | Controlador → ESP32     |
| `NACK`        | Controlador → ESP32     |
| `STATE`       | Controlador → ESP32     |
| `METRIC`      | Controlador → ESP32     |
| `SENSOR`      | Controlador → ESP32     |
| `OUTPUT`      | Controlador → ESP32     |
| `EVENT`       | Controlador → ESP32     |
| `ERROR`       | Controlador → ESP32     |
| `ERROR_CLEAR` | Controlador → ESP32     |
| `CONFIG`      | Ambos / según operación |
| `SNAPSHOT`    | Controlador → ESP32     |

---

# 16. `HELLO`

Formato:

```text
<HELLO,MCU,MODEL,HW_REV,FW_VERSION>
```

Ejemplo:

```text
<HELLO,ATMEGA328P,MAIM_MINI,1.0,1.4.0>
```

Interpretación:

```text
MCU        = ATMEGA328P
MODEL      = MAIM_MINI
HW_REV     = 1.0
FW_VERSION = 1.4.0
```

---

# 17. Propósito de HELLO

Permite al ESP32 conocer:

* microcontrolador;
* modelo;
* revisión de hardware;
* versión de firmware del controlador.

Esto permitirá verificar compatibilidad en futuras versiones.

---

# 18. `STATE`

Formato:

```text
<STATE,KEY,VALUE>
```

Representa un estado lógico actual.

Ejemplos:

```text
<STATE,MODE,STANDBY>
```

```text
<STATE,BUSY,0>
```

---

# 19. Ejemplos de STATE futuros

```text
<STATE,MODE,COOLING>
<STATE,DOOR,OPEN>
<STATE,CHILD_LOCK,1>
```

La estructura no cambia aunque cambie el modelo.

---

# 20. Validación de STATE

Debe contener exactamente:

```text
KEY
VALUE
```

Ejemplo inválido:

```text
<STATE,MODE>
```

El parser puede reconocer sintácticamente la trama, pero `device_manager` debe rechazarla semánticamente.

---

# 21. `METRIC`

Formato:

```text
<METRIC,KEY,VALUE>
```

Representa acumuladores o valores calculados.

Ejemplos:

```text
<METRIC,WATER_TOTAL_ML,16000>
```

```text
<METRIC,LAST_DISPENSE_ML,287>
```

```text
<METRIC,DISPENSE_COUNT,48>
```

---

# 22. Ejemplos METRIC futuros

```text
<METRIC,COMPRESSOR_RUNTIME_SEC,182940>
<METRIC,FILTER_LITERS_REMAINING,1200>
<METRIC,SERVICE_COUNT,4>
```

---

# 23. `SENSOR`

Formato:

```text
<SENSOR,KEY,VALUE>
```

Representa una lectura física.

Ejemplos:

```text
<SENSOR,COLD_TANK_C,7.8>
```

```text
<SENSOR,HOT_TANK_C,81.5>
```

```text
<SENSOR,TANK_LEVEL_PCT,74>
```

---

# 24. Sensor booleano

También puede utilizarse:

```text
<SENSOR,LEAK,0>
```

donde:

```text
0 = sin fuga
1 = fuga detectada
```

---

# 25. `OUTPUT`

Formato:

```text
<OUTPUT,KEY,VALUE>
```

Representa el estado real de un actuador.

Ejemplos:

```text
<OUTPUT,VALVE,1>
```

```text
<OUTPUT,HEATER,0>
```

```text
<OUTPUT,COMPRESSOR,1>
```

---

# 26. Diferencia entre STATE y OUTPUT

```text
STATE
→ condición lógica

OUTPUT
→ actuador físico
```

Ejemplo:

```text
<STATE,MODE,DISPENSING>
```

y simultáneamente:

```text
<OUTPUT,VALVE,1>
```

---

# 27. `EVENT`

Formato:

```text
<EVENT,TYPE[,PARAM1,PARAM2,...]>
```

Representa algo que ocurrió.

Ejemplo:

```text
<EVENT,DISPENSE_STARTED>
```

---

# 28. EVENT con parámetros

Ejemplo:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

Interpretación:

```text
287   → volumen dispensado en ml
16287 → volumen total acumulado en ml
```

---

# 29. Eventos actualmente definidos

```text
DISPENSE_STARTED
DISPENSE_COMPLETED
DISPENSE_CANCELLED
CALIBRATION_COMPLETED
PROGRAMMING_CHANGED
```

---

# 30. `DISPENSE_STARTED`

Formato:

```text
<EVENT,DISPENSE_STARTED>
```

No requiere parámetros.

---

# 31. `DISPENSE_COMPLETED`

Formato:

```text
<EVENT,DISPENSE_COMPLETED,VOLUME_ML,WATER_TOTAL_ML>
```

Ejemplo:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

---

# 32. `DISPENSE_CANCELLED`

Formato:

```text
<EVENT,DISPENSE_CANCELLED,VOLUME_ML>
```

Ejemplo:

```text
<EVENT,DISPENSE_CANCELLED,124>
```

---

# 33. `CALIBRATION_COMPLETED`

Formato:

```text
<EVENT,CALIBRATION_COMPLETED,PULSES_PER_LITER>
```

Ejemplo:

```text
<EVENT,CALIBRATION_COMPLETED,468.2>
```

---

# 34. `PROGRAMMING_CHANGED`

Formato:

```text
<EVENT,PROGRAMMING_CHANGED,DISPENSE_TIME_MS>
```

Ejemplo:

```text
<EVENT,PROGRAMMING_CHANGED,8000>
```

---

# 35. Eventos genéricos

La implementación actual permite recibir eventos no especializados.

Ejemplo:

```text
<EVENT,NEW_EVENT,ABC,123>
```

El ESP32 puede transportarlos temporalmente como parámetros genéricos.

Sin embargo, una vez estabilizado un evento debe documentarse formalmente.

---

# 36. `ERROR`

Formato:

```text
<ERROR,CODE,SEVERITY>
```

Ejemplo:

```text
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

---

# 37. Severidades

Valores oficiales actuales:

```text
INFO
WARNING
ERROR
CRITICAL
```

---

# 38. Ejemplos de ERROR

```text
<ERROR,BATTERY_LOW,WARNING>
```

```text
<ERROR,LEAK_DETECTED,CRITICAL>
```

```text
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

---

# 39. `ERROR_CLEAR`

Formato:

```text
<ERROR_CLEAR,CODE>
```

Ejemplo:

```text
<ERROR_CLEAR,FLOW_SENSOR_FAIL>
```

Indica que el error ya no se encuentra activo.

---

# 40. Por qué existe ERROR_CLEAR

El ESP32 no debe adivinar cuándo una falla desapareció.

El controlador debe informar explícitamente:

```text
ERROR
```

y posteriormente:

```text
ERROR_CLEAR
```

cuando corresponda.

---

# 41. `CMD`

Formato general:

```text
<CMD,TRANSACTION_ID,COMMAND[,PARAM1,PARAM2,...]>
```

Ejemplo:

```text
<CMD,15,DISPENSE>
```

---

# 42. Transaction ID

El campo:

```text
TRANSACTION_ID
```

es un entero interno de UART.

Rango actual:

```text
1 ... 65535
```

---

# 43. Diferencia entre IDs

MQTT utiliza:

```text
CMD-000200
```

UART utiliza:

```text
15
```

`transaction_manager` mantiene la relación:

```text
CMD-000200 ↔ 15
```

---

# 44. `CMD` sin parámetros

Ejemplo:

```text
<CMD,15,DISPENSE>
```

---

# 45. `CMD` con parámetros

Ejemplo conceptual:

```text
<CMD,16,DISPENSE_TIME,5000>
```

---

# 46. Comandos actuales

Actualmente preparados:

```text
DISPENSE
STOP
```

Otros comandos internos del ESP32 como:

```text
PING
GET_STATE
```

no necesitan llegar al controlador.

---

# 47. Comandos futuros posibles

```text
GET_STATE
GET_CONFIG
SET_CONFIG
CALIBRATION
PROGRAMMING
RESET
ENTER_BOOTLOADER
```

Solo deben considerarse oficiales cuando se documenten e implementen.

---

# 48. `ACK`

Formato:

```text
<ACK,TRANSACTION_ID>
```

Ejemplo:

```text
<ACK,15>
```

Significa:

> El controlador recibió y aceptó inicialmente el comando.

---

# 49. ACK no significa finalización

Esta distinción es fundamental.

```text
ACK
→ aceptado
```

no:

```text
ACK
→ terminado
```

La finalización corresponde a:

```text
DONE
```

---

# 50. `DONE`

Formato:

```text
<DONE,TRANSACTION_ID>
```

Ejemplo:

```text
<DONE,15>
```

Significa:

> La operación terminó correctamente.

---

# 51. DONE con resultado

La gramática permite en el futuro:

```text
<DONE,ID,RESULT...>
```

Ejemplo conceptual:

```text
<DONE,15,287>
```

pero para datos relevantes se prefiere utilizar también eventos explícitos.

---

# 52. `NACK`

Formato:

```text
<NACK,TRANSACTION_ID,REASON>
```

Ejemplo:

```text
<NACK,15,BUSY>
```

Significa:

> El controlador recibió la solicitud, pero decidió no ejecutarla.

---

# 53. Razones NACK iniciales

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

---

# 54. Autoridad del controlador

El controlador físico siempre conserva la capacidad de rechazar una orden remota.

Ejemplo:

```text
Servidor solicita DISPENSE
      ↓
ESP32 envía CMD
      ↓
ATmega detecta NO_WATER
      ↓
<NACK,ID,NO_WATER>
```

---

# 55. `CONFIG`

Formato:

```text
<CONFIG,KEY,VALUE>
```

Ejemplo conceptual:

```text
<CONFIG,DISPENSE_TIME_MS,7000>
```

Actualmente se reconoce y valida estructuralmente.

La gestión completa de configuración todavía no está implementada.

---

# 56. Lectura de configuración futura

Ejemplo:

```text
<CMD,300,GET_CONFIG>
```

Respuesta:

```text
<SNAPSHOT,BEGIN,300>
<CONFIG,DISPENSE_TIME_MS,7000>
<CONFIG,PULSES_PER_LITER,468.2>
<SNAPSHOT,END,300>
<DONE,300>
```

Estado:

```text
[PLANEADO]
```

---

# 57. Escritura de configuración futura

Ejemplo:

```text
<CMD,301,SET_CONFIG,DISPENSE_TIME_MS,8000>
```

Respuesta:

```text
<ACK,301>
<DONE,301>
```

o:

```text
<NACK,301,OUT_OF_RANGE>
```

---

# 58. `SNAPSHOT`

Formato:

```text
<SNAPSHOT,BEGIN,ID>
```

y:

```text
<SNAPSHOT,END,ID>
```

Permite delimitar un conjunto coherente de estado.

---

# 59. Ejemplo SNAPSHOT

```text
<CMD,200,GET_STATE>
<ACK,200>
<SNAPSHOT,BEGIN,200>
<STATE,MODE,STANDBY>
<STATE,BUSY,0>
<METRIC,WATER_TOTAL_ML,15420>
<METRIC,LAST_DISPENSE_ML,287>
<OUTPUT,VALVE,0>
<SNAPSHOT,END,200>
<DONE,200>
```

---

# 60. Propósito del Snapshot

Sin snapshot podría ocurrir:

```text
STATE viejo
+
METRIC nuevo
+
OUTPUT todavía no actualizado
```

y publicar una fotografía incoherente.

El snapshot permite saber cuándo se recibió un conjunto completo.

---

# 61. Snapshot actualmente

`device_manager` ya reconoce:

```text
SNAPSHOT BEGIN
SNAPSHOT END
```

y valida que el ID de cierre corresponda al snapshot activo.

La integración completa con publicación diferida de estado puede evolucionar posteriormente.

---

# 62. Parser sintáctico

El módulo:

```text
uart_protocol
```

se encarga únicamente de:

* detectar `<`;
* acumular caracteres;
* detectar `>`;
* validar longitud;
* separar campos;
* identificar tipo de trama.

---

# 63. Parser no valida semántica completa

Ejemplo:

```text
<STATE,MODE,STANBY>
```

es sintácticamente correcta.

El parser no sabe que:

```text
STANBY
```

puede ser un typo de:

```text
STANDBY
```

La validación de significado pertenece a capas superiores.

---

# 64. Separación de responsabilidades

```text
uart_protocol
→ ¿la trama tiene formato válido?

device_manager
→ ¿la trama tiene campos correctos y significado válido?
```

---

# 65. Ejemplo de validación semántica

Trama:

```text
<STATE,MODE>
```

`uart_protocol` puede reconocer:

```text
TYPE = STATE
FIELD_COUNT = 1
```

pero `device_manager` debe responder:

```text
STATE invalido. Se esperaban KEY,VALUE
```

---

# 66. Recuperación ante ruido

El parser utiliza `<` como resincronización.

Ejemplo:

```text
xx!?abc<STATE,MODE,STANDBY>
```

Todo lo anterior a `<` se ignora.

---

# 67. Nueva apertura durante trama

Si aparece un nuevo:

```text
<
```

mientras una trama incompleta estaba en proceso, el parser reinicia el buffer desde ese nuevo inicio.

Esto ayuda a recuperar sincronización.

---

# 68. Trama incompleta

Ejemplo:

```text
<STATE,MODE,STANDBY
```

sin `>`.

No se procesa hasta encontrar cierre o un nuevo inicio de trama.

---

# 69. Overflow

Si una trama supera:

```text
UART_PROTOCOL_MAX_FRAME_LEN
```

se descarta.

El firmware no debe continuar escribiendo fuera del buffer.

---

# 70. Tipo desconocido

Ejemplo:

```text
<LOQUESEA,123>
```

debe producir:

```text
ESP_ERR_NOT_SUPPORTED
```

o comportamiento equivalente.

---

# 71. Campos demasiado largos

Si un campo supera:

```text
UART_PROTOCOL_MAX_FIELD_LEN
```

la trama se rechaza.

---

# 72. Campos vacíos

La versión actual no define campos vacíos como una práctica válida.

Evitar:

```text
<STATE,,1>
```

o:

```text
<CMD,1,,5000>
```

---

# 73. Espacios

No utilizar espacios innecesarios.

Correcto:

```text
<STATE,MODE,STANDBY>
```

No recomendado:

```text
<STATE, MODE, STANDBY>
```

porque el espacio formaría parte del campo.

---

# 74. Caracteres reservados

Los caracteres:

```text
<
>
,
```

tienen significado estructural.

No deben utilizarse dentro de valores normales.

---

# 75. Texto libre

v1 no está diseñado para transportar textos libres arbitrarios con comas.

Si en el futuro se necesita transportar strings complejos deberán evaluarse:

* escaping;
* encoding;
* longitud;
* versión nueva;
* payload binario separado.

---

# 76. CRC / checksum

Actualmente:

```text
NO CRC
```

La primera versión fue diseñada para pistas UART cortas dentro del mismo equipo y para facilitar pruebas.

---

# 77. Motivo de no implementar CRC inicialmente

Primero se priorizó validar:

* gramática;
* semántica;
* eventos;
* comandos;
* transacciones;
* timeouts.

Agregar CRC demasiado pronto habría complicado las pruebas manuales.

---

# 78. CRC futuro

Puede añadirse algo como:

```text
<STATE,MODE,STANDBY*ABCD>
```

o un formato equivalente.

La decisión debe tomarse después de pruebas con hardware físico.

Estado:

```text
[PLANEADO / SEGÚN RESULTADOS]
```

---

# 79. UART físico actual

Todavía no se está utilizando el enlace definitivo ATmega ↔ ESP32 para estas pruebas.

Actualmente la entrada se simula mediante:

```text
idf.py monitor
```

---

# 80. Simulación actual

El usuario escribe manualmente:

```text
<STATE,MODE,STANDBY>
```

y el ESP32 lo procesa exactamente como si hubiera llegado desde el controlador.

---

# 81. Ventaja de esta simulación

Permitió validar:

```text
UART parser
↓
Device Manager
↓
MQTT
```

sin depender todavía de hardware definitivo.

También permitió simular:

```text
ACK
DONE
NACK
```

---

# 82. UART0 actual

La consola ESP-IDF utiliza:

```text
GPIO1 = TX0
GPIO3 = RX0
```

Por eso UART0 está actualmente ocupado por:

* logs;
* monitor;
* entrada manual de pruebas.

---

# 83. UART físico previsto

Para el hardware definitivo se prevé utilizar un UART independiente, conceptualmente:

```text
UART2
```

para:

```text
ESP32 ↔ ATmega
```

y dejar UART0 para:

```text
debug / programación
```

Estado:

```text
[PLANEADO]
```

---

# 84. Baud rate futuro

El baud rate definitivo del enlace aún debe congelarse cuando se integre el hardware físico.

Debe elegirse considerando:

* longitud de pistas;
* frecuencia de CPU;
* estabilidad;
* carga de datos;
* robustez.

---

# 85. Parámetros UART recomendados

Como punto de partida típico:

```text
8 data bits
no parity
1 stop bit
```

pero la configuración física definitiva debe documentarse cuando se implemente.

---

# 86. Device Manager

Una vez parseada la trama, se envía a:

```text
device_manager
```

Ejemplo:

```text
<STATE,MODE,DISPENSING>
```

se guarda como:

```text
MODE = DISPENSING
```

---

# 87. Tablas internas

`device_manager` mantiene categorías:

```text
STATE
METRIC
SENSOR
OUTPUT
ERROR
```

mediante estructura genérica:

```text
KEY → VALUE
```

---

# 88. Por qué los valores se almacenan como texto

UART transporta:

```text
"16000"
"7.8"
"1"
```

como ASCII.

Mantener texto inicialmente permite soportar nuevas claves sin modificar la estructura central.

---

# 89. Conversión tipada

Cuando una capa necesita un tipo concreto utiliza getters.

Ejemplos:

```text
metric → int64
sensor → float
output → bool
```

---

# 90. Eventos y callback

Cuando `device_manager` recibe:

```text
<EVENT,...>
```

invoca un callback hacia la capa superior.

Actualmente ese callback termina en:

```text
mqtt_manager_publish_event()
```

---

# 91. Ruta EVENT completa

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
      ↓
uart_protocol
      ↓
device_manager
      ↓
event callback
      ↓
mqtt_manager
      ↓
Mosquitto
```

---

# 92. Respuestas de transacción

`device_manager` también detecta:

```text
ACK
DONE
NACK
```

y las dirige hacia:

```text
transaction_manager
```

---

# 93. Ruta de ACK

```text
<ACK,15>
   ↓
uart_protocol
   ↓
device_manager
   ↓
transaction_manager
   ↓
MQTT RECEIVED
```

---

# 94. Ruta de DONE

```text
<DONE,15>
   ↓
transaction_manager
   ↓
MQTT SUCCESS
```

---

# 95. Ruta de NACK

```text
<NACK,15,BUSY>
   ↓
transaction_manager
   ↓
MQTT REJECTED
```

---

# 96. ACK Timeout

Si después de:

```text
<CMD,15,DISPENSE>
```

no llega:

```text
<ACK,15>
```

en aproximadamente:

```text
5000 ms
```

la transacción falla con:

```text
ACK_TIMEOUT
```

---

# 97. Execution Timeout

Si llega:

```text
<ACK,15>
```

pero no:

```text
<DONE,15>
```

dentro del intervalo actual:

```text
30000 ms
```

la transacción falla con:

```text
EXECUTION_TIMEOUT
```

---

# 98. Respuesta tardía después de timeout

Una vez liberada una transacción por timeout, una respuesta posterior como:

```text
<DONE,15>
```

debe considerarse una respuesta para una transacción desconocida.

No debe reactivar la operación MQTT anterior.

---

# 99. IDs duplicados

El `transaction_manager` genera IDs internos progresivos.

La secuencia evita utilizar:

```text
0
```

como ID normal.

Después de `65535` puede volver a:

```text
1
```

siempre que no exista colisión con una transacción activa.

La estrategia futura puede reforzarse si fuese necesario.

---

# 100. ACK duplicado

La implementación actual detecta ACK duplicados cuando la transacción ya está:

```text
ACKED
```

y puede ignorarlos.

Esto mejora tolerancia ante respuestas repetidas.

---

# 101. DONE sin transacción

Ejemplo:

```text
<DONE,999>
```

cuando no existe ID 999 activo.

Debe registrarse como:

```text
respuesta para transaccion desconocida
```

y no generar SUCCESS MQTT.

---

# 102. NACK sin transacción

Mismo principio:

```text
<NACK,999,BUSY>
```

no debe asociarse a una solicitud inexistente.

---

# 103. Seguridad física

El protocolo nunca debe permitir que el ESP32 sustituya las validaciones del controlador.

Ejemplo:

```text
<CMD,15,DISPENSE>
```

es una solicitud.

El controlador debe decidir si:

```text
agua disponible
equipo libre
sin errores críticos
seguridad cumplida
```

antes de ejecutar.

---

# 104. El protocolo transporta intención

Regla:

> CMD expresa intención; no acceso directo a GPIO.

Evitar diseños tipo:

```text
<CMD,GPIO8,1>
```

desde el servidor para funciones físicas críticas.

Preferir:

```text
<CMD,15,DISPENSE>
```

y que el controlador gestione los actuadores adecuados.

---

# 105. Comandos abstractos

Esto permite cambiar internamente:

```text
válvula
bomba
tiempo
secuencia
```

sin modificar el API remoto.

---

# 106. Compatibilidad entre modelos

Un equipo simple puede utilizar:

```text
<STATE,MODE,STANDBY>
<OUTPUT,VALVE,0>
```

y uno complejo:

```text
<SENSOR,HOT_TANK_C,81.5>
<SENSOR,COLD_TANK_C,7.8>
<OUTPUT,COMPRESSOR,1>
<OUTPUT,HEATER,0>
```

sin cambiar la gramática.

---

# 107. Claves específicas de producto

Las claves individuales pueden depender del producto.

La gramática:

```text
<SENSOR,KEY,VALUE>
```

no cambia.

---

# 108. Catálogo futuro de claves

Será conveniente mantener un catálogo por modelo o familia.

Ejemplo:

```text
MAIM MINI
→ VALVE
→ WATER_TOTAL_ML

MAIM ELITE
→ COLD_TANK_C
→ HOT_TANK_C
→ COMPRESSOR
```

pero esto no pertenece a la gramática base.

---

# 109. Versionado

Esta especificación corresponde a:

```text
MAIM Internal UART Protocol v1
```

Agregar nuevas claves o eventos compatibles no requiere necesariamente v2.

---

# 110. Cambio compatible

Ejemplos:

```text
nuevo SENSOR
nuevo METRIC
nuevo EVENT
nuevo COMMAND
```

pueden añadirse dentro de v1 si respetan la gramática y no cambian significados existentes.

---

# 111. Cambio incompatible

Podría requerir:

```text
v2
```

si se modifica:

* delimitación fundamental;
* significado de una clase;
* tipos incompatibles;
* sistema de transacciones de forma no compatible.

---

# 112. No reutilizar claves con otro significado

Si:

```text
WATER_TOTAL_ML
```

significa mililitros acumulados, nunca debe reutilizarse para litros.

---

# 113. No cambiar unidad silenciosamente

Incorrecto:

```text
WATER_TOTAL_ML
```

pero enviar litros.

Si cambia la unidad, utilizar una nueva clave.

---

# 114. Reglas de extensibilidad

Los receptores deben:

* aceptar tipos conocidos;
* rechazar tipos desconocidos de forma controlada;
* ignorar ruido fuera de tramas;
* no bloquearse por una trama inválida;
* recuperar sincronización en la siguiente `<`.

---

# 115. Robustez ante errores

Nunca una trama inválida debe:

* provocar overflow;
* bloquear permanentemente el parser;
* ejecutar comandos parciales;
* alterar actuadores directamente;
* corromper memoria.

---

# 116. Flujo de recepción

```text
carácter UART
   ↓
uart_protocol_process_char()
   ↓
buffer
   ↓
>
   ↓
parse_frame()
   ↓
uart_protocol_frame_t
   ↓
callback
```

---

# 117. Estructura parseada

Conceptualmente:

```c
typedef struct
{
    uart_frame_type_t type;
    uint8_t field_count;
    char fields[MAX_FIELDS][MAX_FIELD_LEN];

} uart_protocol_frame_t;
```

---

# 118. Tipos enumerados internos

Actualmente:

```text
UART_FRAME_HELLO
UART_FRAME_CMD
UART_FRAME_ACK
UART_FRAME_DONE
UART_FRAME_NACK
UART_FRAME_STATE
UART_FRAME_METRIC
UART_FRAME_SENSOR
UART_FRAME_OUTPUT
UART_FRAME_EVENT
UART_FRAME_ERROR
UART_FRAME_ERROR_CLEAR
UART_FRAME_CONFIG
UART_FRAME_SNAPSHOT
```

---

# 119. Generación de comandos

El ESP32 utiliza:

```c
uart_protocol_build_command(...)
```

para construir tramas.

Ejemplo:

```text
ID = 15
COMMAND = DISPENSE
```

produce:

```text
<CMD,15,DISPENSE>
```

---

# 120. Simulación manual validada

Se probaron satisfactoriamente:

```text
<STATE,MODE,STANDBY>
<METRIC,WATER_TOTAL_ML,15420>
<SENSOR,COLD_TANK_C,7.8>
<OUTPUT,VALVE,1>
<EVENT,DISPENSE_COMPLETED,287,15707>
<ERROR,FLOW_SENSOR_FAIL,ERROR>
<ACK,101>
<DONE,101>
<NACK,102,BUSY>
```

---

# 121. Resultado del parser

Ejemplo:

```text
Trama valida | Tipo=STATE | Campos=2
Campo[0] = MODE
Campo[1] = STANDBY
```

---

# 122. Validación de Device Manager

Posteriormente se comprobó que las tramas actualizaban correctamente:

```text
STATE
METRICS
SENSORS
OUTPUTS
ERRORS
```

y podían ser utilizadas para construir MQTT.

---

# 123. Prueba completa UART → MQTT

Entrada:

```text
<STATE,MODE,DISPENSING>
<STATE,BUSY,1>
<METRIC,WATER_TOTAL_ML,16000>
<METRIC,LAST_DISPENSE_ML,287>
<METRIC,DISPENSE_COUNT,48>
<OUTPUT,VALVE,1>
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

Posteriormente `GET_STATE` produjo un `state/reported` con esos valores.

Estado:

```text
[PROBADO]
```

---

# 124. Prueba EVENT inmediata

Entrada:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

produjo inmediatamente un MQTT event.

Estado:

```text
[PROBADO]
```

---

# 125. Prueba ruta inversa

MQTT:

```text
DISPENSE
```

ESP32 generó:

```text
<CMD,ID,DISPENSE>
```

Después se simularon:

```text
<ACK,ID>
<DONE,ID>
```

y MQTT produjo:

```text
RECEIVED
SUCCESS
```

Estado:

```text
[PROBADO]
```

---

# 126. Prueba NACK

Entrada:

```text
<NACK,ID,BUSY>
```

produjo:

```text
REJECTED
BUSY
```

Estado:

```text
[PROBADO]
```

---

# 127. Prueba ACK Timeout

No enviar:

```text
<ACK,ID>
```

después de CMD.

Resultado:

```text
FAILED
ACK_TIMEOUT
```

Estado:

```text
[PROBADO]
```

---

# 128. Prueba Execution Timeout

Enviar:

```text
<ACK,ID>
```

pero no:

```text
<DONE,ID>
```

Resultado:

```text
FAILED
EXECUTION_TIMEOUT
```

Estado:

```text
[PROBADO]
```

---

# 129. Timeouts no forman parte del UART puro

Los valores:

```text
5000 ms
30000 ms
```

pertenecen a `transaction_manager`.

No están codificados dentro de la trama UART.

---

# 130. Timeouts específicos futuros

En el futuro diferentes comandos pueden requerir diferentes tiempos.

Ejemplo:

```text
STOP
→ corto

CALIBRATION
→ largo

FIRMWARE_UPDATE
→ mucho más largo
```

Estado:

```text
[PLANEADO]
```

---

# 131. Flujo completo controlador → servidor

```text
Controlador
   │
   │ UART frame
   ▼
uart_protocol
   │
   ▼
device_manager
   │
   ▼
mqtt_manager
   │
   ▼
Mosquitto
```

---

# 132. Flujo completo servidor → controlador

```text
Mosquitto
   │
   ▼
mqtt_manager
   │
   ▼
transaction_manager
   │
   ▼
uart_protocol_build_command()
   │
   ▼
UART
   │
   ▼
Controlador
```

---

# 133. ACK/DONE/NACK de regreso

```text
Controlador
   │
   ├── ACK
   ├── DONE
   └── NACK
   │
   ▼
uart_protocol
   │
   ▼
device_manager
   │
   ▼
transaction_manager
   │
   ▼
mqtt_manager
```

---

# 134. Futuro UART Manager

Actualmente `uart_protocol` no administra directamente el periférico UART.

Se prevé crear:

```text
uart_manager.c
uart_manager.h
```

---

# 135. Responsabilidad futura de uart_manager

```text
configurar UART2
leer bytes
escribir bytes
gestionar RX
gestionar TX
entregar caracteres al parser
```

---

# 136. Separación futura

```text
uart_manager
→ hardware

uart_protocol
→ gramática

device_manager
→ significado
```

Esta separación debe conservarse.

---

# 137. Posible arquitectura futura

```text
UART2 driver
    ↓
uart_manager
    ↓
uart_protocol
    ↓
device_manager
```

En transmisión:

```text
transaction_manager
    ↓
uart_protocol
    ↓
uart_manager
    ↓
UART2
```

---

# 138. Buffering futuro

Cuando exista hardware físico debe evaluarse:

* ring buffer;
* tarea RX;
* eventos UART;
* tamaño de buffer;
* overflow;
* pérdida de bytes.

---

# 139. Frecuencia de mensajes

El controlador no debe inundar UART con datos sin necesidad.

Puede utilizarse:

* cambio de estado;
* evento inmediato;
* telemetría interna periódica razonable;
* snapshot bajo solicitud.

---

# 140. No duplicar telemetría innecesariamente

Ejemplo:

```text
<SENSOR,COLD_TANK_C,7.8>
```

cada pocos milisegundos no tiene sentido si la temperatura cambia lentamente.

La frecuencia debe diseñarse según cada dato.

---

# 141. Eventos críticos

Los eventos importantes deben transmitirse inmediatamente.

Ejemplo:

```text
<ERROR,LEAK_DETECTED,CRITICAL>
```

no debe esperar un snapshot periódico.

---

# 142. Priorización futura

UART v1 no incluye prioridad explícita.

Si en el futuro la carga aumenta, puede añadirse lógica en `uart_manager` para priorizar:

```text
ERROR
EVENT
ACK/NACK/DONE
```

sobre telemetría menos importante.

---

# 143. Handshake futuro

`HELLO` puede utilizarse durante arranque para confirmar que ambos procesadores están comunicándose.

Ejemplo:

```text
ESP32 boot
  ↓
espera HELLO
  ↓
controlador identificado
```

---

# 144. Compatibilidad futura mediante HELLO

Puede verificarse:

```text
MODEL
HW_REV
FW_VERSION
```

antes de habilitar determinadas funciones.

---

# 145. Firmware AVR futuro

Cuando el ESP32 pueda actualizar el ATmega, el protocolo UART normal puede utilizarse para coordinar:

```text
preparar actualización
entrar en modo seguro
verificar versión
reiniciar controlador
```

La programación ISP en sí utilizará otro mecanismo físico.

---

# 146. No utilizar UART para secretos en texto claro

El protocolo actual no está cifrado.

Aunque sea una conexión interna de PCB, no debe utilizarse innecesariamente para transportar:

* contraseñas;
* claves privadas;
* tokens secretos.

---

# 147. Seguridad interna futura

Si futuros productos requieren protección adicional ante manipulación física, puede evaluarse autenticación o integridad de mensajes.

Actualmente:

```text
[NO IMPLEMENTADO]
```

---

# 148. Tabla resumen de clases

| Tipo          | Campos principales  | Función          |
| ------------- | ------------------- | ---------------- |
| `HELLO`       | MCU, MODEL, HW, FW  | Identificación   |
| `STATE`       | KEY, VALUE          | Estado lógico    |
| `METRIC`      | KEY, VALUE          | Métrica          |
| `SENSOR`      | KEY, VALUE          | Sensor           |
| `OUTPUT`      | KEY, VALUE          | Actuador         |
| `EVENT`       | TYPE, PARAMS        | Evento           |
| `ERROR`       | CODE, SEVERITY      | Error activo     |
| `ERROR_CLEAR` | CODE                | Limpiar error    |
| `CMD`         | ID, COMMAND, PARAMS | Solicitud        |
| `ACK`         | ID                  | Aceptado         |
| `DONE`        | ID                  | Terminado        |
| `NACK`        | ID, REASON          | Rechazado        |
| `CONFIG`      | KEY, VALUE          | Configuración    |
| `SNAPSHOT`    | BEGIN/END, ID       | Estado coherente |

---

# 149. Ejemplos de referencia

```text
<HELLO,ATMEGA328P,MAIM_MINI,1.0,1.4.0>

<STATE,MODE,STANDBY>
<STATE,BUSY,0>

<METRIC,WATER_TOTAL_ML,16000>
<METRIC,LAST_DISPENSE_ML,287>

<SENSOR,COLD_TANK_C,7.8>

<OUTPUT,VALVE,1>

<EVENT,DISPENSE_STARTED>
<EVENT,DISPENSE_COMPLETED,287,16287>

<ERROR,FLOW_SENSOR_FAIL,ERROR>
<ERROR_CLEAR,FLOW_SENSOR_FAIL>

<CMD,15,DISPENSE>

<ACK,15>
<DONE,15>

<NACK,15,BUSY>

<SNAPSHOT,BEGIN,200>
<SNAPSHOT,END,200>
```

---

# 150. Reglas principales del protocolo

1. Toda trama inicia con `<`.
2. Toda trama termina con `>`.
3. Los campos se separan con `,`.
4. No utilizar comas dentro de valores.
5. Utilizar ASCII.
6. Utilizar `UPPER_SNAKE_CASE` para identificadores.
7. Utilizar punto decimal.
8. Utilizar `0/1` para booleanos.
9. Incluir unidades en claves cuando corresponda.
10. No exceder longitud máxima.
11. No ejecutar tramas inválidas.
12. El controlador conserva autoridad física.
13. ACK no significa DONE.
14. NACK debe incluir una razón.
15. Los IDs relacionan respuestas con comandos.
16. Los eventos son hechos, no estado.
17. Los errores activos requieren `ERROR_CLEAR`.
18. El parser debe recuperar sincronización automáticamente.
19. Los cambios compatibles deben mantener v1.
20. Cambios incompatibles requieren una nueva versión.

---

# 151. Estado actual

```text
[PROBADO] delimitadores < >
[PROBADO] parsing por coma
[PROBADO] STATE
[PROBADO] METRIC
[PROBADO] SENSOR
[PROBADO] OUTPUT
[PROBADO] EVENT
[PROBADO] ERROR
[PROBADO] ERROR_CLEAR
[PROBADO] ACK
[PROBADO] DONE
[PROBADO] NACK
[PROBADO] SNAPSHOT parsing
[PROBADO] Device Manager
[PROBADO] UART → MQTT
[PROBADO] MQTT → UART simulado
[PROBADO] ACK Timeout
[PROBADO] Execution Timeout

[EN DESARROLLO] catálogo final de comandos por producto

[PLANEADO] uart_manager físico
[PLANEADO] UART2
[PLANEADO] HELLO automático
[PLANEADO] GET_STATE automático al boot
[PLANEADO] CONFIG completo
[PLANEADO] CRC según pruebas físicas
[PLANEADO] actualización AVR
```

---

# 152. Relación con otros documentos

Arquitectura:

```text
01_ARQUITECTURA_GENERAL.md
```

MQTT:

```text
08_PROTOCOLO_MQTT.md
```

Device Manager:

```text
10_DEVICE_MANAGER.md
```

Eventos:

```text
11_EVENTOS.md
```

Transacciones:

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

# 153. Resumen

MAIM Internal UART Protocol v1 establece una interfaz común:

```text
Controlador principal
        │
        │ <TYPE,...>
        ▼
       ESP32
```

basada en clases genéricas:

```text
STATE
METRIC
SENSOR
OUTPUT
EVENT
ERROR
CMD
ACK
DONE
NACK
```

La gramática está diseñada para mantenerse estable incluso cuando los futuros equipos MAIM incorporen más sensores, actuadores, funciones o procesos.

Esta especificación constituye la referencia base para toda comunicación interna **ESP32 ↔ controlador principal** dentro de MAIM Connectivity v0.1.0.
