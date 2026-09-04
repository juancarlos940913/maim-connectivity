# MAIM Connectivity

## Timeouts y Supervisión de Transacciones

**Documento:** `13_TIMEOUTS.md`
**Proyecto:** MAIM Connectivity
**Módulo principal:** `transaction_manager`
**Módulos relacionados:** `mqtt_manager`, `uart_protocol`, `device_manager`
**Estado:** [PROBADO] ACK Timeout y Execution Timeout funcionales

---

# 1. Propósito

Este documento describe el sistema de **timeouts utilizado para supervisar las transacciones entre el ESP32 y el controlador principal**.

El objetivo es evitar que una solicitud enviada desde el servidor permanezca indefinidamente esperando una respuesta del controlador.

La arquitectura debe poder detectar situaciones como:

```text
ESP32 envía comando
        ↓
ATmega no responde
```

o:

```text
ESP32 envía comando
        ↓
ATmega responde ACK
        ↓
ATmega nunca termina la operación
```

Sin un mecanismo de timeout, cualquiera de estas situaciones podría dejar una transacción ocupando recursos permanentemente.

---

# 2. Problema que resuelven los timeouts

Una comunicación UART no garantiza por sí misma que el controlador:

```text
esté funcionando
haya recibido correctamente el mensaje
pueda ejecutar la operación
termine la operación
```

Por ello, cada transacción debe tener límites temporales.

La regla general es:

> **Toda transacción debe terminar correctamente, ser rechazada o expirar.**

Nunca debe permanecer activa indefinidamente.

---

# 3. Timeouts actuales

Actualmente se utilizan dos mecanismos principales:

```text
ACK_TIMEOUT
```

y:

```text
EXECUTION_TIMEOUT
```

Sus funciones son diferentes.

---

# 4. ACK Timeout

El **ACK Timeout** supervisa cuánto tiempo puede esperar el ESP32 a que el controlador confirme que recibió el comando.

Flujo:

```text
ESP32
  │
  │ <CMD,ID,COMMAND>
  ▼
ATmega
  │
  │ debe responder
  ▼
<ACK,ID>
```

Si ACK no llega dentro del tiempo permitido:

```text
ACK_TIMEOUT
```

---

# 5. Tiempo actual de ACK

Actualmente:

```text
ACK_TIMEOUT = aproximadamente 5 segundos
```

Este tiempo comienza a contar después de crear y enviar correctamente la transacción UART.

---

# 6. Ejemplo normal

ESP32 envía:

```text
<CMD,15,DISPENSE>
```

Controlador responde:

```text
<ACK,15>
```

antes de 5 segundos.

Resultado:

```text
WAITING_ACK
     ↓
    ACK
     ↓
   ACKED
```

El ACK Timeout deja de aplicar.

---

# 7. Ejemplo de ACK Timeout

ESP32:

```text
<CMD,15,DISPENSE>
```

No se recibe ninguna respuesta.

Después de aproximadamente:

```text
5 segundos
```

se genera:

```text
ACK_TIMEOUT
```

La transacción cambia a:

```text
FAILED
```

---

# 8. Respuesta MQTT por ACK Timeout

El servidor recibe conceptualmente:

```json
{
  "schema": 1,
  "command_id": "CMD-TIMEOUT-003",
  "ts": 1786500000,
  "status": "FAILED",
  "error": {
    "code": "ACK_TIMEOUT"
  }
}
```

Esto permite distinguir entre:

```text
el controlador rechazó el comando
```

y:

```text
el controlador nunca respondió
```

---

# 9. Significado de ACK_TIMEOUT

`ACK_TIMEOUT` significa:

> **El ESP32 transmitió la solicitud hacia el controlador, pero no recibió confirmación dentro del tiempo permitido.**

No significa necesariamente que el ATmega esté averiado.

Puede deberse a:

```text
controlador bloqueado
reinicio del controlador
problema UART
trama perdida
firmware detenido
error eléctrico
controlador desconectado
```

---

# 10. Estado después de ACK Timeout

La transición es:

```text
WAITING_ACK
     │
     │ tiempo > ACK_TIMEOUT
     ▼
   FAILED
```

Después:

```text
publicar FAILED
↓
liberar transacción
```

---

# 11. Liberación del slot

Una vez generado:

```text
ACK_TIMEOUT
```

la transacción no debe seguir ocupando un slot.

Conceptualmente:

```text
used = false
```

Esto permite reutilizar el espacio para futuras solicitudes.

---

# 12. Execution Timeout

El segundo mecanismo es:

```text
EXECUTION_TIMEOUT
```

Este timeout comienza **después de recibir ACK**.

Su objetivo es detectar operaciones que fueron aceptadas por el controlador pero nunca finalizaron.

---

# 13. Ejemplo

ESP32 envía:

```text
<CMD,20,DISPENSE>
```

Controlador responde:

```text
<ACK,20>
```

Esto demuestra que:

```text
el controlador está vivo
+
recibió el comando
+
aceptó inicialmente la operación
```

Pero todavía falta:

```text
<DONE,20>
```

---

# 14. Tiempo actual de ejecución

Actualmente:

```text
EXECUTION_TIMEOUT = aproximadamente 30 segundos
```

El contador correspondiente comienza cuando se recibe:

```text
ACK
```

---

# 15. Ejemplo normal de ejecución

```text
<CMD,20,DISPENSE>
```

↓

```text
<ACK,20>
```

↓

el controlador realiza la operación.

↓

```text
<DONE,20>
```

Resultado:

```text
ACKED
  ↓
DONE
  ↓
COMPLETED
```

No se genera timeout.

---

# 16. Ejemplo de Execution Timeout

ESP32:

```text
<CMD,20,DISPENSE>
```

Controlador:

```text
<ACK,20>
```

pero nunca:

```text
<DONE,20>
```

Después de aproximadamente:

```text
30 segundos
```

se genera:

```text
EXECUTION_TIMEOUT
```

---

# 17. Respuesta MQTT por Execution Timeout

Ejemplo conceptual:

```json
{
  "schema": 1,
  "command_id": "CMD-TIMEOUT-004",
  "ts": 1786500000,
  "status": "FAILED",
  "error": {
    "code": "EXECUTION_TIMEOUT"
  }
}
```

---

# 18. Significado de EXECUTION_TIMEOUT

Significa:

> **El controlador confirmó que recibió la operación, pero no informó su finalización dentro del tiempo esperado.**

Esto puede indicar:

```text
operación bloqueada
máquina de estados detenida
sensor esperando indefinidamente
fallo del controlador
problema UART posterior al ACK
reinicio durante ejecución
DONE perdido
```

---

# 19. Diferencia entre ambos timeouts

La diferencia es fundamental:

```text
ACK_TIMEOUT
```

significa:

```text
"No sé si el controlador recibió el comando."
```

Mientras:

```text
EXECUTION_TIMEOUT
```

significa:

```text
"Sé que el controlador recibió y aceptó el comando,
pero no confirmó que terminó."
```

---

# 20. Línea temporal completa

```text
        CMD enviado
            │
            ▼
      WAITING_ACK
            │
            │
       ACK_TIMEOUT
         ~5 s
            │
      ┌─────┴─────┐
      │           │
     ACK      no responde
      │           │
      ▼           ▼
    ACKED       FAILED
      │       ACK_TIMEOUT
      │
      │
 EXECUTION_TIMEOUT
      ~30 s
      │
 ┌────┴────┐
 │         │
DONE   no termina
 │         │
 ▼         ▼
SUCCESS   FAILED
          EXECUTION_TIMEOUT
```

---

# 21. Máquina de estados temporal

```text
                 CREATE
                    │
                    ▼
               WAITING_ACK
                 /     \
                /       \
              ACK      5 s
               │         │
               ▼         ▼
             ACKED     FAILED
               │     ACK_TIMEOUT
               │
               │
              30 s
               │
          ┌────┴────┐
          │         │
        DONE      TIMEOUT
          │         │
          ▼         ▼
      COMPLETED   FAILED
                  EXECUTION_TIMEOUT
```

---

# 22. Medición de tiempo

Los timeouts no utilizan el reloj Unix.

Se utiliza el temporizador monotónico del ESP32 mediante:

```c
esp_timer_get_time()
```

---

# 23. Unidad de `esp_timer_get_time()`

La función devuelve tiempo en:

```text
microsegundos
```

Por ello:

```text
1 segundo = 1,000,000 µs
```

---

# 24. Ejemplo

Cinco segundos:

```c
5 * 1000000
```

Treinta segundos:

```c
30 * 1000000
```

---

# 25. Por qué no utilizar Unix timestamp

El timestamp Unix depende de la sincronización SNTP.

Durante el arranque puede ocurrir:

```text
ts = 0
```

o la hora todavía podría no estar sincronizada.

Los timeouts internos no deben depender de Internet.

---

# 26. Reloj monotónico

`esp_timer_get_time()` permite medir:

```text
tiempo transcurrido desde el arranque
```

independientemente de:

```text
WiFi
MQTT
SNTP
Internet
zona horaria
```

Esto lo hace apropiado para timeouts internos.

---

# 27. Separación de relojes

La arquitectura utiliza dos conceptos diferentes.

### Tiempo absoluto

```text
time_manager_get_timestamp()
```

Uso:

```text
MQTT
eventos
telemetría
historial
```

### Tiempo monotónico

```text
esp_timer_get_time()
```

Uso:

```text
timeouts
temporizadores internos
duraciones
```

---

# 28. Regla

> **Los timestamps describen cuándo ocurrió algo. Los timers determinan cuánto tiempo ha pasado.**

No deben intercambiarse.

---

# 29. Registro de creación

Cuando se crea una transacción se registra:

```text
created_us
```

Conceptualmente:

```c
transaction->created_us = esp_timer_get_time();
```

---

# 30. Uso de `created_us`

Mientras:

```text
state == WAITING_ACK
```

se calcula:

```text
ahora - created_us
```

Si supera:

```text
ACK_TIMEOUT
```

la transacción falla.

---

# 31. Registro de ACK

Cuando llega:

```text
<ACK,ID>
```

se registra:

```text
ack_received_us
```

Conceptualmente:

```c
transaction->ack_received_us = esp_timer_get_time();
```

---

# 32. Uso de `ack_received_us`

Mientras:

```text
state == ACKED
```

se calcula:

```text
ahora - ack_received_us
```

Si supera:

```text
EXECUTION_TIMEOUT
```

se genera:

```text
EXECUTION_TIMEOUT
```

---

# 33. Por qué reiniciar la referencia temporal en ACK

No sería correcto calcular el Execution Timeout desde:

```text
created_us
```

porque el tiempo que tardó el controlador en enviar ACK consumiría parte del tiempo disponible para ejecutar la operación.

Por ello:

```text
CMD
↓
created_us
↓
ACK
↓
ack_received_us
↓
EXECUTION_TIMEOUT
```

---

# 34. Supervisión periódica

No se utiliza un `delay()` de 5 o 30 segundos esperando respuestas.

Eso bloquearía el sistema.

La supervisión se realiza periódicamente.

---

# 35. Tarea supervisora

El `transaction_manager` utiliza una tarea que revisa las transacciones activas.

Conceptualmente:

```text
transaction_timeout_task
```

Su función es:

```text
revisar slots
↓
identificar estado
↓
calcular tiempo transcurrido
↓
detectar expiraciones
```

---

# 36. Periodo de revisión

Actualmente la tarea revisa aproximadamente cada:

```text
250 ms
```

Esto significa que un timeout configurado a 5 segundos puede detectarse ligeramente después del valor exacto.

Ejemplo:

```text
5.00 s
5.08 s
5.17 s
5.24 s
```

Esto es normal.

---

# 37. No es un temporizador de precisión

Los timeouts son mecanismos de supervisión.

No están diseñados para ejecutar una acción exactamente en:

```text
5.000000 segundos
```

El objetivo es detectar fallas de comunicación.

---

# 38. Implementación no bloqueante

Conceptualmente:

```c
while (1)
{
    transaction_manager_check_timeouts();

    vTaskDelay(pdMS_TO_TICKS(250));
}
```

Esto permite que simultáneamente continúen funcionando:

```text
WiFi
MQTT
UART
telemetría
eventos
device_manager
```

---

# 39. Revisión de slots

La tarea recorre la tabla de transacciones.

Conceptualmente:

```c
for (int i = 0; i < MAX_ACTIVE; i++)
{
    if (!transactions[i].used)
    {
        continue;
    }

    // revisar timeout
}
```

---

# 40. Slot WAITING_ACK

Si:

```text
state == WAITING_ACK
```

se verifica:

```text
now - created_us
```

contra:

```text
ACK_TIMEOUT
```

---

# 41. Slot ACKED

Si:

```text
state == ACKED
```

se verifica:

```text
now - ack_received_us
```

contra:

```text
EXECUTION_TIMEOUT
```

---

# 42. Estados que no requieren timeout

Una transacción que ya está:

```text
COMPLETED
REJECTED
FAILED
```

no debe continuar siendo supervisada.

Normalmente su slot ya habrá sido liberado.

---

# 43. Flujo ACK Timeout

```text
transaction_manager
      │
      ▼
state = WAITING_ACK
      │
      ▼
elapsed > ACK_TIMEOUT
      │
      ▼
state = FAILED
      │
      ▼
callback
      │
      ▼
mqtt_manager
      │
      ▼
FAILED / ACK_TIMEOUT
      │
      ▼
liberar slot
```

---

# 44. Flujo Execution Timeout

```text
transaction_manager
      │
      ▼
state = ACKED
      │
      ▼
elapsed > EXECUTION_TIMEOUT
      │
      ▼
state = FAILED
      │
      ▼
callback
      │
      ▼
mqtt_manager
      │
      ▼
FAILED / EXECUTION_TIMEOUT
      │
      ▼
liberar slot
```

---

# 45. Callback

El `transaction_manager` no debe construir directamente el JSON MQTT.

En su lugar notifica:

```text
mqtt_command_id
state
reason
```

Ejemplo:

```text
CMD-TIMEOUT-003
FAILED
ACK_TIMEOUT
```

---

# 46. Responsabilidad de `mqtt_manager`

`mqtt_manager` convierte ese resultado en:

```json
{
  "status": "FAILED",
  "error": {
    "code": "ACK_TIMEOUT"
  }
}
```

Esto mantiene desacopladas las capas.

---

# 47. Prueba manual de ACK Timeout

La prueba realizada consiste en:

```text
1. Publicar un comando MQTT.
2. ESP32 crea la transacción.
3. ESP32 genera CMD UART.
4. No enviar ACK manualmente.
5. Esperar aproximadamente 5 segundos.
6. Observar ACK_TIMEOUT.
7. Confirmar FAILED en MQTT.
```

Estado:

```text
[PROBADO]
```

---

# 48. Ejemplo de prueba

Servidor:

```json
{
  "schema": 1,
  "command_id": "CMD-TIMEOUT-003",
  "command": "DISPENSE",
  "params": {}
}
```

ESP32 genera algo equivalente a:

```text
<CMD,3,DISPENSE>
```

No se responde.

Resultado:

```text
FAILED
ACK_TIMEOUT
```

---

# 49. Prueba manual de Execution Timeout

Procedimiento:

```text
1. Publicar comando MQTT.
2. Observar UART transaction ID.
3. Enviar manualmente <ACK,ID>.
4. Confirmar MQTT RECEIVED.
5. No enviar DONE.
6. Esperar aproximadamente 30 segundos.
7. Confirmar EXECUTION_TIMEOUT.
8. Confirmar MQTT FAILED.
```

Estado:

```text
[PROBADO]
```

---

# 50. Prueba de operación correcta

También se comprobó el caso contrario.

```text
1. Publicar comando.
2. Enviar ACK.
3. Enviar DONE.
4. Confirmar SUCCESS.
5. Esperar más que el Execution Timeout.
```

Resultado:

```text
NO aparece EXECUTION_TIMEOUT
```

Estado:

```text
[PROBADO]
```

---

# 51. Importancia de esta prueba

No basta con comprobar que el timeout funciona.

También es necesario comprobar que:

> **una transacción completada correctamente deja de estar supervisada.**

De lo contrario podría ocurrir:

```text
SUCCESS
↓
30 segundos
↓
FAILED
```

lo cual sería un error grave.

---

# 52. Cancelación del ACK Timeout

Cuando llega:

```text
<ACK,ID>
```

la transacción cambia:

```text
WAITING_ACK
↓
ACKED
```

Por tanto:

```text
ACK_TIMEOUT
```

ya no puede activarse para esa transacción.

---

# 53. Cancelación del Execution Timeout

Cuando llega:

```text
<DONE,ID>
```

la transacción:

```text
ACKED
↓
COMPLETED
```

y posteriormente se libera.

Por tanto:

```text
EXECUTION_TIMEOUT
```

ya no puede generarse.

---

# 54. NACK y timeout

Si llega:

```text
<NACK,ID,BUSY>
```

antes de que expire ACK Timeout:

```text
WAITING_ACK
↓
REJECTED
```

La transacción termina inmediatamente.

No debe generarse posteriormente:

```text
ACK_TIMEOUT
```

---

# 55. Respuesta tardía después de ACK Timeout

Escenario:

```text
<CMD,40,DISPENSE>
↓
5 segundos
↓
ACK_TIMEOUT
↓
slot liberado
↓
<ACK,40>
```

Ese ACK llegó demasiado tarde.

---

# 56. Política de respuesta tardía

El sistema no debe:

```text
recrear la transacción
cambiar FAILED por RECEIVED
ejecutar otra acción
```

Debe simplemente reconocer que:

```text
UART ID 40
```

ya no corresponde a una transacción activa.

---

# 57. Resultado esperado

Conceptualmente:

```text
Warning:
ACK para transaction ID desconocido
```

y se ignora la respuesta.

---

# 58. DONE tardío

Mismo principio.

```text
EXECUTION_TIMEOUT
↓
slot liberado
↓
<DONE,40>
```

El DONE no debe convertir posteriormente la solicitud en:

```text
SUCCESS
```

---

# 59. NACK tardío

También:

```text
ACK_TIMEOUT
↓
<NACK,40,BUSY>
```

no debe cambiar el resultado ya publicado.

---

# 60. Transacción cerrada

Regla:

> **Una transacción cerrada no vuelve a abrirse debido a una respuesta UART tardía.**

---

# 61. Riesgo de reutilización de UART ID

Los IDs UART son:

```text
uint16_t
```

y eventualmente pueden reutilizarse.

Esto crea un escenario teórico:

```text
ID 40 antiguo expira
↓
mucho tiempo después
↓
ID 40 se reutiliza
↓
llega respuesta extremadamente tardía del ID antiguo
```

---

# 62. Riesgo actual

En condiciones normales esto es poco probable porque existen:

```text
65535 IDs
```

antes de volver a utilizar el mismo número.

Sin embargo, debe mantenerse documentado.

---

# 63. Estrategias futuras

Si fuera necesario aumentar robustez:

```text
generation counter
session ID
boot ID
IDs más grandes
ventana de reutilización
```

Estado:

```text
[PLANEADO / SOLO SI ES NECESARIO]
```

---

# 64. Timeout no significa cancelar físicamente

Este punto es fundamental.

Si ocurre:

```text
EXECUTION_TIMEOUT
```

el ESP32 sabe que perdió confirmación de la operación.

Eso **no garantiza que el controlador haya dejado de ejecutar físicamente la operación**.

---

# 65. Ejemplo crítico

```text
DISPENSE
↓
ACK
↓
ATmega abre válvula
↓
UART deja de funcionar
↓
ESP32 genera EXECUTION_TIMEOUT
```

La válvula podría continuar controlada por el ATmega.

Por eso:

> **La seguridad física debe permanecer en el controlador principal.**

---

# 66. Timeouts físicos independientes

El ATmega debe mantener sus propios límites de seguridad.

Ejemplo:

```text
dispensado máximo
protección de bomba
protección de válvula
temperatura máxima
tiempo máximo de calentamiento
```

Estos límites no deben depender del ESP32.

---

# 67. Dos niveles de timeout

La arquitectura debe diferenciar:

```text
TIMEOUT DE COMUNICACIÓN
```

y:

```text
TIMEOUT DE SEGURIDAD FÍSICA
```

---

# 68. Timeout de comunicación

Responsabilidad:

```text
ESP32
transaction_manager
```

Pregunta:

```text
¿El controlador respondió?
```

---

# 69. Timeout de seguridad

Responsabilidad:

```text
ATmega
```

Pregunta:

```text
¿Esta operación física puede continuar de forma segura?
```

---

# 70. Ejemplo

ESP32:

```text
EXECUTION_TIMEOUT = 30 s
```

ATmega:

```text
DISPENSE_MAX_TIME = valor definido por lógica del equipo
```

Son mecanismos independientes.

---

# 71. No utilizar ESP32 como única seguridad

Nunca debe diseñarse:

```text
ESP32 timeout
↓
cerrar GPIO directamente
```

como única protección.

El controlador principal debe poder proteger el equipo aunque:

```text
ESP32 se reinicie
WiFi falle
MQTT falle
UART falle
servidor desaparezca
```

---

# 72. Timeouts específicos por comando

Actualmente se utilizan valores generales.

Sin embargo, diferentes operaciones pueden necesitar tiempos diferentes.

Ejemplo:

```text
STOP
```

debería finalizar rápidamente.

Mientras:

```text
CALIBRATION
```

podría requerir mucho más tiempo.

---

# 73. Ejemplo futuro

```text
COMMAND               ACK        EXECUTION

DISPENSE              5 s        30 s
STOP                   3 s         5 s
CALIBRATION            5 s       180 s
PROGRAMMING            5 s       120 s
RESET_TOTAL            5 s        15 s
```

Estos valores son conceptuales y deberán definirse según el comportamiento real de cada equipo.

---

# 74. Estado actual

```text
timeouts generales
```

Estado:

```text
[IMPLEMENTADO]
```

```text
timeouts específicos por comando
```

Estado:

```text
[PLANEADO]
```

---

# 75. Heartbeat no sustituye timeout

El hecho de que el controlador envíe periódicamente información no sustituye:

```text
ACK
DONE
```

de una transacción.

---

# 76. Ejemplo

Aunque el ESP32 siga recibiendo:

```text
STATE
METRIC
SENSOR
```

si nunca recibe:

```text
ACK,15
```

la transacción:

```text
15
```

debe generar:

```text
ACK_TIMEOUT
```

---

# 77. Estado global vs transacción

Un controlador puede estar:

```text
ONLINE
```

pero una transacción específica puede:

```text
TIMEOUT
```

Son conceptos distintos.

---

# 78. MQTT online no significa controlador online

Del mismo modo:

```text
MQTT = ONLINE
```

solo indica que el ESP32 está conectado al broker.

No demuestra necesariamente que el ATmega esté respondiendo.

---

# 79. Detección futura del controlador

Más adelante puede implementarse:

```text
heartbeat UART
```

o:

```text
PING interno
```

para determinar explícitamente:

```text
controller_online
```

Estado:

```text
[PLANEADO]
```

---

# 80. Timeout de MQTT

Los timeouts documentados aquí corresponden principalmente a:

```text
ESP32 ↔ controlador
```

No deben confundirse con:

```text
MQTT keepalive
Mosquitto offline detection
WiFi reconnect
TCP timeout
```

---

# 81. Mosquitto offline detection

Actualmente se decidió permitir que Mosquitto determine la desconexión del ESP32 mediante su comportamiento normal de MQTT/LWT.

Ese mecanismo es independiente del:

```text
transaction_manager
```

---

# 82. Capas de supervisión

La arquitectura tiene diferentes niveles:

```text
WiFi
↓
supervisa conectividad de red

MQTT
↓
supervisa conexión con broker

LWT / Keepalive
↓
supervisa disponibilidad ESP32

Transaction Manager
↓
supervisa respuestas del controlador

ATmega
↓
supervisa seguridad física
```

---

# 83. No mezclar timeouts

Cada capa debe mantener sus propios tiempos.

Por ejemplo:

```text
MQTT offline
```

no debe generar automáticamente:

```text
ACK_TIMEOUT
```

si no existe una transacción UART activa.

---

# 84. Reinicio del controlador

Si el ATmega se reinicia después de recibir CMD pero antes de responder:

```text
CMD
↓
ATmega reboot
↓
sin ACK
```

ESP32 generará:

```text
ACK_TIMEOUT
```

Esto es correcto desde la perspectiva de comunicación.

---

# 85. Reinicio después de ACK

Escenario:

```text
CMD
↓
ACK
↓
ATmega reboot
↓
sin DONE
```

ESP32 generará:

```text
EXECUTION_TIMEOUT
```

También es correcto.

---

# 86. Reinicio del ESP32

Si el ESP32 reinicia:

```text
transaction_manager
```

pierde actualmente las transacciones activas almacenadas en RAM.

Estado:

```text
[LIMITACIÓN CONOCIDA]
```

---

# 87. Persistencia

Actualmente no se persisten las transacciones en NVS.

Esto evita complejidad y escrituras frecuentes en Flash.

---

# 88. Estrategia futura ante reboot

Podría implementarse:

```text
BOOT
↓
sincronización con ATmega
↓
GET_STATE
↓
reconciliación
```

pero esto deberá diseñarse cuando exista el controlador físico definitivo.

---

# 89. Watchdog vs timeout

No deben confundirse.

Un:

```text
WATCHDOG
```

detecta software que dejó de ejecutarse correctamente.

Un:

```text
TRANSACTION TIMEOUT
```

detecta una operación que no recibió la respuesta esperada.

---

# 90. Ejemplo

Si `transaction_manager` sigue funcionando pero el ATmega no responde:

```text
watchdog
→ todo normal

transaction timeout
→ falla
```

---

# 91. Otro ejemplo

Si una tarea del ESP32 se bloquea completamente:

```text
watchdog
→ puede intervenir
```

El timeout podría no ejecutarse si su propia tarea dejó de recibir CPU.

Por ello son protecciones complementarias.

---

# 92. Logs recomendados

ACK Timeout:

```text
TRANSACTION_MANAGER:
ACK timeout
UART ID=15
MQTT ID=CMD-TIMEOUT-003
```

Execution Timeout:

```text
TRANSACTION_MANAGER:
Execution timeout
UART ID=16
MQTT ID=CMD-TIMEOUT-004
```

---

# 93. Información útil de diagnóstico

Idealmente cada log debe permitir identificar:

```text
UART ID
MQTT command_id
command
state
tipo de timeout
tiempo transcurrido
```

---

# 94. No exponer detalles innecesarios

MQTT no necesita conocer:

```text
slot interno
punteros
created_us
ack_received_us
```

Estos pertenecen al diagnóstico interno del ESP32.

---

# 95. MQTT debe recibir semántica

El servidor necesita:

```text
FAILED
ACK_TIMEOUT
```

o:

```text
FAILED
EXECUTION_TIMEOUT
```

No necesita conocer la implementación interna.

---

# 96. Tabla resumen

| Estado actual | Espera                                  | Tiempo aproximado | Si vence            |
| ------------- | --------------------------------------- | ----------------: | ------------------- |
| `WAITING_ACK` | `ACK` / `NACK`                          |               5 s | `ACK_TIMEOUT`       |
| `ACKED`       | `DONE` / posible `NACK` según protocolo |              30 s | `EXECUTION_TIMEOUT` |
| `COMPLETED`   | Nada                                    |                 — | Liberar             |
| `REJECTED`    | Nada                                    |                 — | Liberar             |
| `FAILED`      | Nada                                    |                 — | Liberar             |

---

# 97. Tabla de respuestas

| Respuesta UART        | Estado anterior | Resultado                    |
| --------------------- | --------------- | ---------------------------- |
| `ACK`                 | `WAITING_ACK`   | `ACKED`                      |
| `NACK`                | `WAITING_ACK`   | `REJECTED`                   |
| `DONE`                | `ACKED`         | `COMPLETED`                  |
| Ninguna por ~5 s      | `WAITING_ACK`   | `FAILED / ACK_TIMEOUT`       |
| Ningún DONE por ~30 s | `ACKED`         | `FAILED / EXECUTION_TIMEOUT` |

---

# 98. Casos de prueba mínimos

Antes de considerar estable una modificación de `transaction_manager`, deben probarse al menos:

```text
CMD → ACK → DONE
CMD → NACK
CMD → sin respuesta
CMD → ACK → sin DONE
CMD → ACK → DONE → esperar >30 s
ACK desconocido
DONE desconocido
NACK desconocido
```

---

# 99. Prueba 1 — éxito

```text
CMD
↓
ACK
↓
DONE
```

Esperado:

```text
RECEIVED
SUCCESS
```

Sin timeout posterior.

---

# 100. Prueba 2 — rechazo

```text
CMD
↓
NACK BUSY
```

Esperado:

```text
REJECTED
BUSY
```

Sin timeout posterior.

---

# 101. Prueba 3 — controlador sin respuesta

```text
CMD
↓
...
```

Esperado:

```text
~5 s
↓
FAILED
ACK_TIMEOUT
```

---

# 102. Prueba 4 — controlador bloqueado durante operación

```text
CMD
↓
ACK
↓
...
```

Esperado:

```text
~30 s
↓
FAILED
EXECUTION_TIMEOUT
```

---

# 103. Prueba 5 — DONE antes del límite

```text
CMD
↓
ACK
↓
29 s
↓
DONE
```

Esperado:

```text
SUCCESS
```

No debe aparecer:

```text
EXECUTION_TIMEOUT
```

---

# 104. Prueba 6 — respuesta tardía

```text
CMD
↓
ACK_TIMEOUT
↓
ACK
```

Esperado:

```text
ACK desconocido
```

Sin modificar el resultado anterior.

---

# 105. Prueba 7 — múltiples transacciones

Crear varias solicitudes activas.

Cada una debe mantener independientemente:

```text
UART ID
created_us
ack_received_us
state
```

Un timeout en una no debe afectar las demás.

---

# 106. Concurrencia

La tarea de supervisión puede revisar una transacción al mismo tiempo que otra tarea procesa:

```text
ACK
DONE
NACK
```

Por ello, al evolucionar el sistema deberá mantenerse atención sobre acceso concurrente a la tabla de transacciones.

---

# 107. Protección de la tabla

Si futuras pruebas muestran condiciones de carrera, puede ser necesario utilizar:

```text
mutex
critical section
```

al modificar entradas.

Estado:

```text
[REVISAR SEGÚN CREZCA EL SISTEMA]
```

---

# 108. No agregar bloqueos innecesarios

Mientras el sistema actual funcione correctamente, no deben introducirse mecanismos de sincronización complejos sin necesidad.

La arquitectura debe mantenerse:

```text
simple
predecible
diagnosticable
```

---

# 109. Evolución futura

Las siguientes mejoras pueden evaluarse:

```text
timeouts por comando
heartbeat ATmega
detección controller_online
estadísticas de timeout
contador de fallas UART
reintentos controlados
reconciliación después de reboot
```

---

# 110. Reintentos automáticos

Actualmente un timeout produce:

```text
FAILED
```

No debe asumirse que el comando puede repetirse automáticamente.

---

# 111. Por qué no reintentar DISPENSE automáticamente

Escenario:

```text
ESP32 envía DISPENSE
↓
ATmega lo recibe
↓
ACK se pierde
↓
ESP32 genera ACK_TIMEOUT
```

Si ESP32 reenvía automáticamente:

```text
DISPENSE
```

el controlador podría ejecutar la operación dos veces.

---

# 112. Regla de reintentos

> **Los comandos que producen acciones físicas no deben reintentarse automáticamente sin un mecanismo de idempotencia.**

---

# 113. Ejemplo seguro

Un comando como:

```text
GET_STATE
```

puede repetirse con mucho menor riesgo.

---

# 114. Ejemplo peligroso

```text
DISPENSE
RESET_TOTAL
START_CALIBRATION
```

no deben repetirse ciegamente.

---

# 115. Relación con deduplicación

Cuando se implemente deduplicación de:

```text
command_id
```

podrán evaluarse estrategias más robustas de reintento.

Hasta entonces:

```text
timeout
→ FAILED
```

y el servidor decide qué hacer.

---

# 116. Métricas futuras

Sería útil registrar:

```text
ack_timeout_count
execution_timeout_count
nack_count
successful_transaction_count
```

Esto permitirá diagnosticar equipos en campo.

Estado:

```text
[PLANEADO]
```

---

# 117. Ejemplo diagnóstico

Un equipo con:

```text
ACK_TIMEOUT = 150
```

pero:

```text
MQTT estable
WiFi RSSI bueno
```

podría indicar un problema entre:

```text
ESP32 ↔ ATmega
```

---

# 118. Otro ejemplo

Muchos:

```text
EXECUTION_TIMEOUT
```

podrían indicar problemas en:

```text
máquina de estados ATmega
sensores
actuadores
lógica de finalización
```

---

# 119. Estado actual del sistema

Actualmente:

```text
[IMPLEMENTADO] ACK Timeout
[IMPLEMENTADO] Execution Timeout
[IMPLEMENTADO] temporizador monotónico
[IMPLEMENTADO] tarea de supervisión
[IMPLEMENTADO] liberación después de timeout
[IMPLEMENTADO] callback de fallo
[IMPLEMENTADO] publicación MQTT FAILED
```

---

# 120. Validación realizada

Se verificó manualmente:

```text
[PROBADO] CMD sin ACK → ACK_TIMEOUT
[PROBADO] CMD + ACK sin DONE → EXECUTION_TIMEOUT
[PROBADO] CMD + ACK + DONE → SUCCESS
[PROBADO] no aparece timeout después de SUCCESS
[PROBADO] respuesta MQTT contiene timestamp
```

---

# 121. Pendientes

```text
[PLANEADO] timeouts específicos por comando
[PLANEADO] heartbeat controlador
[PLANEADO] controller_online
[PLANEADO] métricas de fallos
[PLANEADO] estrategia de recuperación tras reboot
[PLANEADO] deduplicación de command_id
[PLANEADO] estudiar reintentos seguros
```

---

# 122. Principios de diseño

1. Toda transacción debe terminar o expirar.
2. Ninguna espera debe ser infinita.
3. ACK Timeout y Execution Timeout representan fallas diferentes.
4. Los timeouts internos deben utilizar reloj monotónico.
5. SNTP no debe utilizarse para medir duraciones internas.
6. Los timeouts deben implementarse de forma no bloqueante.
7. Una transacción completada debe dejar de ser supervisada.
8. Una transacción rechazada debe liberarse.
9. Una transacción expirada debe liberarse.
10. Las respuestas tardías no deben reabrir transacciones.
11. Los timeouts de comunicación no sustituyen seguridad física.
12. El controlador principal debe mantener sus propios límites de seguridad.
13. MQTT online no significa controlador online.
14. Un timeout no demuestra necesariamente que una acción física no ocurrió.
15. No deben existir reintentos automáticos de acciones físicas sin idempotencia.
16. Los diferentes tipos de comandos podrán requerir diferentes timeouts.
17. Los logs deben permitir diagnosticar qué transacción expiró.
18. Los errores de timeout deben propagarse al servidor de forma semántica.
19. Cada capa debe supervisar únicamente las responsabilidades que le corresponden.
20. El sistema debe fallar de manera controlada y observable.

---

# 123. Arquitectura final resumida

```text
              SERVIDOR
                  │
                  │ command/request
                  ▼
                MQTT
                  │
                  ▼
                ESP32
                  │
          transaction_manager
                  │
                  ▼
          <CMD,ID,COMMAND>
                  │
                  ▼
               ATmega
               /    \
              /      \
           ACK       nada
            │          │
            ▼          │
          ACKED        │
            │          │
       ┌────┴────┐     │
       │         │     │
     DONE      30 s   5 s
       │         │     │
       ▼         ▼     ▼
   COMPLETED   FAILED FAILED
                │       │
          EXEC_TIMEOUT ACK_TIMEOUT
```

---

# 124. Relación con otros documentos

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

Eventos:

```text
11_EVENTOS.md
```

Comandos y transacciones:

```text
12_COMANDOS_Y_TRANSACCIONES.md
```

Pruebas:

```text
14_PRUEBAS_Y_DIAGNOSTICO.md
```

---

# 125. Resumen

El sistema de timeouts proporciona una capa fundamental de tolerancia a fallos entre el ESP32 y el controlador principal.

Actualmente existen dos supervisiones:

```text
CMD
↓
esperar ACK
↓
~5 segundos
↓
ACK_TIMEOUT
```

y:

```text
ACK
↓
esperar DONE
↓
~30 segundos
↓
EXECUTION_TIMEOUT
```

Los tiempos se calculan utilizando:

```c
esp_timer_get_time()
```

y son revisados periódicamente sin bloquear el resto del firmware.

El resultado de una expiración sigue la ruta:

```text
transaction_manager
↓
FAILED
↓
mqtt_manager
↓
command/response
↓
Servidor
```

Esto permite distinguir entre un controlador que **no respondió** y uno que **aceptó una operación pero nunca confirmó su finalización**.

Las pruebas realizadas hasta este punto han confirmado correctamente:

```text
CMD → ACK → DONE
        ↓
      SUCCESS

CMD → sin ACK
        ↓
    ACK_TIMEOUT

CMD → ACK → sin DONE
        ↓
 EXECUTION_TIMEOUT
```

Con esto, **MAIM Connectivity v0.1.0** ya dispone de una base transaccional capaz de detectar y reportar bloqueos de comunicación sin dejar solicitudes pendientes indefinidamente.

---

**Fin del documento — `13_TIMEOUTS.md`**
