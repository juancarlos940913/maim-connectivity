# MAIM Connectivity

## Sincronización de Hora

**Documento:** 07_SINCRONIZACION_HORA.md
**Proyecto:** MAIM Connectivity
**Módulo:** `time_manager`
**Framework:** ESP-IDF 5.5.5
**Mecanismo:** SNTP
**Estado:** [PROBADO] Sincronización y timestamp Unix funcionales

---

# 1. Propósito

Este documento describe cómo **MAIM Connectivity** obtiene, mantiene y utiliza la hora dentro del ESP32.

La sincronización de tiempo es necesaria principalmente para:

* timestamps MQTT;
* eventos;
* telemetría;
* estados reportados;
* respuestas de comandos;
* registros históricos;
* diagnóstico;
* futuras funciones de actualización y seguridad.

El módulo responsable es:

```text
time_manager.c
time_manager.h
```

---

# 2. Principio general

El ESP32 no dispone de una referencia absoluta de fecha/hora válida después de un arranque en frío.

Por ello, una vez disponible la conectividad Wi-Fi, se utiliza:

```text
SNTP
```

para obtener la hora desde servidores NTP.

La secuencia actual es:

```text
ESP32 arranca
   ↓
NVS
   ↓
Wi-Fi
   ↓
obtiene IP
   ↓
SNTP
   ↓
reloj del sistema válido
   ↓
MQTT
```

---

# 3. Posición dentro de la arquitectura

```text
Internet
   │
   │ NTP/SNTP
   ▼
time_manager
   │
   ▼
Reloj del ESP32
   │
   ├── MQTT timestamps
   ├── eventos
   ├── state/reported
   ├── telemetry
   └── command/response
```

El módulo MQTT no administra directamente la sincronización.

Solicita únicamente:

```c
time_manager_get_timestamp();
```

---

# 4. Responsabilidades de `time_manager`

El módulo administra:

```text
Inicialización SNTP
Servidor NTP
Espera de sincronización
Validación de hora
Timestamp Unix
Hora local para diagnóstico
Estado de sincronización
```

---

# 5. Dependencia de Wi-Fi

SNTP necesita conectividad de red.

Por ello `time_manager_init()` se ejecuta únicamente después de:

```text
wifi_manager_is_connected() == true
```

La secuencia correcta es:

```text
Wi-Fi
  ↓
IP válida
  ↓
SNTP
```

---

# 6. Implementación actual

La inicialización utiliza la infraestructura:

```text
esp_netif_sntp
```

de ESP-IDF.

Conceptualmente:

```c
esp_sntp_config_t config =
    ESP_NETIF_SNTP_DEFAULT_CONFIG(
        "pool.ntp.org"
    );

esp_netif_sntp_init(
    &config
);
```

Posteriormente se espera la sincronización:

```c
esp_netif_sntp_sync_wait(
    pdMS_TO_TICKS(15000)
);
```

---

# 7. Servidor NTP actual

Durante las pruebas se utilizó:

```text
pool.ntp.org
```

Inicialmente también se consideró:

```text
time.google.com
```

pero apareció un problema de configuración relacionado con la cantidad máxima de servidores SNTP permitidos por lwIP.

---

# 8. Problema encontrado: demasiados servidores SNTP

El error observado fue:

```text
Tried to configure more servers than enabled in lwip.
Please update CONFIG_SNTP_MAX_SERVERS
```

seguido de:

```text
Failed initialize SNTP service
```

y:

```text
ESP_ERR_INVALID_ARG
```

---

# 9. Diagnóstico del error

Este error no significaba que:

```text
pool.ntp.org
```

o:

```text
time.google.com
```

estuvieran caídos.

La falla ocurría antes de realizar una consulta real de red.

El problema era:

```text
CONFIG_SNTP_MAX_SERVERS
```

configurado con un número inferior al requerido por la configuración utilizada.

---

# 10. Solución

Abrir:

```powershell
idf.py menuconfig
```

buscar:

```text
SNTP_MAX_SERVERS
```

utilizando:

```text
/
```

y aumentar el número máximo permitido.

Después guardar la configuración.

---

# 11. Verificación de configuración

Puede buscarse dentro de:

```text
sdkconfig
```

una opción equivalente a:

```text
CONFIG_LWIP_SNTP_MAX_SERVERS
```

El nombre exacto puede depender de la configuración de ESP-IDF, pero debe permitir al menos el número de servidores configurados.

---

# 12. Resultado después de corregir configuración

Una vez ajustado `menuconfig`, la sincronización funcionó correctamente.

El firmware comenzó a producir timestamps como:

```text
1786488124
```

Ejemplo real de telemetría:

```json
{
  "schema": 1,
  "ts": 1786488124,
  "network": {
    "rssi_dbm": -59
  }
}
```

Antes de la sincronización se observaba:

```json
{
  "ts": 0
}
```

---

# 13. Timestamp Unix

MAIM Connectivity utiliza:

```text
Unix timestamp
```

expresado en:

```text
segundos desde 1970-01-01 00:00:00 UTC
```

Ejemplo:

```text
1786497569
```

---

# 14. Uso de UTC

Los timestamps del protocolo MQTT deben interpretarse como:

```text
UTC
```

No deben depender directamente de la zona horaria local del equipo.

Esto permite que dispositivos ubicados en diferentes países utilicen la misma base temporal.

---

# 15. Ejemplo conceptual

Dos dispositivos:

```text
México
China
```

pueden reportar:

```text
ts = 1786497569
```

para representar exactamente el mismo instante.

El backend puede posteriormente convertir ese timestamp a la zona horaria deseada.

---

# 16. Zona horaria local

La zona horaria se utiliza únicamente para:

* logs legibles;
* diagnóstico;
* interfaces locales;
* visualización.

No modifica el significado del Unix timestamp.

---

# 17. Configuración local utilizada

Durante laboratorio se utilizó:

```c
setenv(
    "TZ",
    "CST6",
    1
);

tzset();
```

Esto permitió imprimir una hora local aproximada correspondiente a México central.

---

# 18. Consideración futura sobre zonas horarias

Para una plataforma internacional será preferible asociar la ubicación del dispositivo con una zona IANA, por ejemplo:

```text
America/Mexico_City
America/Tijuana
America/New_York
Asia/Shanghai
```

La presentación local debe realizarse preferentemente en backend o interfaz.

---

# 19. Validación de hora

El módulo comprueba que el reloj tenga un valor razonable antes de declararlo válido.

Conceptualmente:

```text
si timestamp > fecha mínima conocida
→ hora válida
```

Durante desarrollo se utilizó un umbral posterior a 2024.

Esto evita aceptar valores cercanos a:

```text
1970-01-01
```

como sincronización correcta.

---

# 20. `time_manager_is_synced()`

La función:

```c
time_manager_is_synced();
```

permite saber si existe una referencia temporal válida.

Resultado:

```text
true
```

cuando el reloj ya se encuentra sincronizado o contiene una hora válida.

---

# 21. `time_manager_get_timestamp()`

La función principal es:

```c
time_manager_get_timestamp();
```

Retorna:

```text
int64_t
```

con Unix timestamp.

Si todavía no existe una hora válida puede devolver:

```text
0
```

---

# 22. `time_manager_get_local_time()`

Para diagnóstico se dispone de una función que convierte el reloj del sistema a texto local.

Ejemplo:

```text
2026-08-11 16:45:20
```

Esta representación no se utiliza como valor principal dentro de MQTT.

---

# 23. Por qué no enviar fecha como texto

No se utiliza como formato principal:

```text
2026-08-11 16:45:20
```

porque:

* ocupa más bytes;
* requiere conocer zona horaria;
* puede ser ambiguo;
* complica comparaciones;
* complica almacenamiento;
* complica ordenamiento.

El timestamp Unix es más adecuado para protocolo.

---

# 24. Uso en MQTT

Actualmente se utiliza `ts` en:

```text
availability
telemetry
state/reported
event
command/response
```

Ejemplo:

```json
{
  "schema": 1,
  "ts": 1786497569
}
```

---

# 25. Availability

Ejemplo:

```json
{
  "schema": 1,
  "ts": 1786497569,
  "status": "online"
}
```

---

# 26. Telemetry

Ejemplo:

```json
{
  "schema": 1,
  "ts": 1786497569,
  "network": {
    "rssi_dbm": -54
  }
}
```

---

# 27. State Reported

Ejemplo:

```json
{
  "schema": 1,
  "ts": 1786497569,
  "state": {
    "mode": "STANDBY"
  }
}
```

---

# 28. Event

Ejemplo:

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

# 29. Command Response

Ejemplo:

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "ts": 1786504167,
  "status": "SUCCESS"
}
```

---

# 30. Tiempo absoluto vs tiempo monotónico

MAIM Connectivity utiliza dos conceptos diferentes de tiempo.

## Tiempo absoluto

Proviene de:

```text
SNTP
```

y se utiliza para:

```text
timestamps
históricos
eventos
logs remotos
```

## Tiempo monotónico

Proviene de:

```c
esp_timer_get_time();
```

y se utiliza para:

```text
timeouts
duraciones
temporizadores internos
```

---

# 31. Regla arquitectónica

Nunca utilizar el reloj SNTP para medir:

```text
ACK Timeout
Execution Timeout
debounce
intervalos internos
duraciones críticas
```

porque la hora absoluta puede ajustarse.

---

# 32. `esp_timer_get_time()`

`transaction_manager` utiliza:

```c
esp_timer_get_time();
```

Este valor representa microsegundos transcurridos desde el arranque.

Ejemplo conceptual:

```text
123456789 us
```

No representa una fecha.

---

# 33. Ventaja del reloj monotónico

Si SNTP corrige el reloj:

```text
+2 segundos
```

o:

```text
-1 segundo
```

un timeout interno no debe verse afectado.

Por ello:

```text
SNTP
≠
timeout clock
```

---

# 34. Ejemplo ACK Timeout

Al enviar:

```text
<CMD,15,DISPENSE>
```

se registra:

```text
created_us
```

con:

```c
esp_timer_get_time();
```

Después se compara con:

```text
5000 ms
```

No se utiliza `time(NULL)`.

---

# 35. Ejemplo Execution Timeout

Cuando llega:

```text
<ACK,15>
```

se registra:

```text
ack_received_us
```

Después:

```text
30 segundos
```

sin DONE generan:

```text
EXECUTION_TIMEOUT
```

independientemente de cualquier ajuste SNTP.

---

# 36. Qué ocurre después de sincronizar

Una vez que SNTP ajusta el reloj del sistema, el ESP32 puede continuar incrementándolo localmente.

No necesita consultar un servidor NTP para cada mensaje MQTT.

Conceptualmente:

```text
SNTP
  ↓
sincroniza reloj
  ↓
reloj continúa avanzando localmente
```

---

# 37. Pérdida temporal de Internet

Si el ESP32 ya se sincronizó y después pierde Internet:

```text
última sincronización válida
        ↓
reloj continúa
```

por lo que los timestamps pueden seguir generándose.

La precisión podrá degradarse lentamente por deriva del reloj, pero no regresará inmediatamente a cero.

---

# 38. Reinicio completo sin Internet

Escenario:

```text
ESP32 apagado
   ↓
arranca
   ↓
Wi-Fi sin Internet
   ↓
SNTP no puede sincronizar
```

En este caso puede no existir una referencia absoluta válida.

El firmware actual puede continuar, pero:

```text
ts
```

puede ser:

```text
0
```

hasta que exista sincronización.

---

# 39. Filosofía de tolerancia a falla

Una falla SNTP no debe impedir:

```text
arranque del ESP32
Wi-Fi
MQTT local
comunicación UART
control físico del equipo
```

Durante el desarrollo se comprobó esta separación.

Cuando SNTP falló apareció:

```text
TIME_MANAGER: Error inicializando SNTP
MAIM: Continuando sin hora sincronizada
```

y MQTT continuó arrancando.

---

# 40. Resultado observado durante falla SNTP

El ESP32 siguió:

```text
Wi-Fi conectado
MQTT conectado
subscriptions activas
```

pero publicó:

```json
{
  "ts": 0
}
```

Esto confirmó que el módulo estaba desacoplado.

---

# 41. Ventaja arquitectónica

Una falla de un servicio no debe derribar servicios no relacionados.

Ejemplo:

```text
SNTP falla
   │
   ├── MQTT puede continuar
   ├── UART continúa
   ├── Device Manager continúa
   └── control físico continúa
```

---

# 42. Tiempo máximo de espera inicial

Actualmente se espera aproximadamente:

```text
15 segundos
```

mediante:

```c
esp_netif_sntp_sync_wait(
    pdMS_TO_TICKS(15000)
);
```

Si no se obtiene hora en ese intervalo, la inicialización no bloquea indefinidamente.

---

# 43. Por qué no esperar indefinidamente

Esperar para siempre una respuesta NTP podría provocar:

```text
ESP32 bloqueado en arranque
MQTT nunca inicia
servicios posteriores no arrancan
```

Por ello se utiliza un timeout.

---

# 44. Posible mejora futura

Puede implementarse una estrategia:

```text
SNTP inicial falla
      ↓
continuar arranque
      ↓
reintentar en background
```

Estado:

```text
[PLANEADO]
```

Esto permitiría que un equipo que arranque sin Internet obtenga hora automáticamente cuando la conectividad regrese.

---

# 45. Sincronización periódica

SNTP puede realizar sincronizaciones posteriores para corregir deriva.

La política definitiva de resincronización todavía no se ha congelado para producción.

Debe equilibrar:

```text
precisión
tráfico
consumo energético
complejidad
```

---

# 46. Relación con batería

En un dispositivo alimentado por batería no conviene despertar Wi-Fi únicamente para obtener la hora con excesiva frecuencia.

Cuando se implemente power management habrá que evaluar:

```text
intervalo de sincronización
deriva aceptable
deep sleep
retención RTC
costo energético
```

Estado:

```text
[PLANEADO]
```

---

# 47. Relación futura con RTC

Si futuros equipos necesitan mantener hora precisa durante periodos largos completamente desconectados, puede evaluarse:

```text
RTC externo
```

con batería propia.

Actualmente no es necesario para la arquitectura base.

---

# 48. SNTP y MQTT

MQTT debe inicializarse después del intento de sincronización.

Secuencia:

```text
Wi-Fi
 ↓
SNTP
 ↓
MQTT
```

Esto permite que el primer:

```text
availability
```

pueda contener un timestamp real cuando la red tiene Internet.

---

# 49. Consideración sobre LAN sin Internet

En el laboratorio el broker MQTT se encuentra en la LAN.

Es posible tener:

```text
Wi-Fi ✓
Mosquitto LAN ✓
Internet ✗
SNTP ✗
```

En ese caso MQTT puede funcionar aunque SNTP no pueda consultar Internet.

Esta es otra razón para mantener ambos módulos desacoplados.

---

# 50. Posible servidor NTP local futuro

Para instalaciones aisladas podría existir:

```text
NTP local MAIM
```

o el propio servidor de infraestructura podría actuar como referencia temporal.

Estado:

```text
[PLANEADO / SEGÚN ARQUITECTURA]
```

---

# 51. Validación desde monitor serie

Después de flashear:

```powershell
idf.py -p COM6 flash monitor
```

debe aparecer:

```text
TIME_MANAGER: Inicializando sincronizacion SNTP
TIME_MANAGER: Esperando sincronizacion de hora...
```

y posteriormente:

```text
TIME_MANAGER: Hora sincronizada: ...
TIME_MANAGER: Unix timestamp: ...
```

---

# 52. Validación mediante MQTT

En Rocky:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/<DEVICE_ID>/telemetry" \
-v
```

Debe aparecer:

```json
{
  "schema": 1,
  "ts": 17865xxxxx,
  "network": {
    "rssi_dbm": -54
  }
}
```

---

# 53. Criterio de timestamp válido

Para operación normal:

```text
ts > 0
```

y debe corresponder aproximadamente con la fecha actual.

Un:

```text
ts = 0
```

indica que la hora absoluta aún no se encuentra disponible.

---

# 54. Comprobar timestamp externamente

Durante diagnóstico puede convertirse un Unix timestamp mediante:

* backend;
* herramientas de desarrollo;
* PowerShell;
* Python;
* sistemas Linux.

No es necesario hacerlo dentro del firmware para cada mensaje.

---

# 55. Conversión en PowerShell

Ejemplo:

```powershell
[DateTimeOffset]::FromUnixTimeSeconds(1786497569)
```

Esto muestra el instante correspondiente en UTC.

Para hora local:

```powershell
[DateTimeOffset]::FromUnixTimeSeconds(1786497569).ToLocalTime()
```

---

# 56. Conversión en Linux

Ejemplo:

```bash
date -d @1786497569
```

El resultado se mostrará utilizando la zona horaria configurada en Linux.

---

# 57. Timestamp y base de datos

El backend deberá decidir cómo almacenar el tiempo.

Recomendación arquitectónica:

```text
almacenar UTC
```

y convertir a zona local únicamente para presentación.

---

# 58. Ventajas de UTC en backend

Evita problemas con:

* cambio de zona horaria;
* horario de verano;
* instalaciones internacionales;
* usuarios en diferentes regiones;
* orden cronológico.

---

# 59. Horario de verano

No debe codificarse manualmente dentro del protocolo MQTT.

Las reglas de timezone pueden cambiar.

La conversión de presentación debe usar bases de datos de zonas horarias actualizadas.

---

# 60. No usar timestamp generado por ATmega

Actualmente el ATmega no necesita mantener fecha/hora absoluta.

Ejemplo:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

no contiene timestamp.

El ESP32 lo agrega al recibirlo.

---

# 61. Ventaja de centralizar tiempo en ESP32

Esto evita cargar al controlador principal con:

```text
NTP
RTC
fecha
zona horaria
Unix conversion
```

El ATmega se concentra en el control físico.

---

# 62. Precisión de eventos

El timestamp del evento representa aproximadamente:

> El instante en el que el ESP32 recibió y procesó la trama del controlador.

No necesariamente el microsegundo exacto en el que ocurrió físicamente la acción.

Para los eventos actuales esta precisión es suficiente.

---

# 63. Futuro: timestamps del controlador

Si alguna aplicación futura requiere precisión temporal muy estricta entre el evento físico y el servidor, podría extenderse el protocolo UART.

Ejemplo conceptual:

```text
<EVENT,...,LOCAL_COUNTER>
```

pero actualmente no es necesario.

---

# 64. Event ID y tiempo

Los eventos contienen:

```text
event_id
ts
```

Son conceptos distintos.

`event_id` identifica el mensaje.

`ts` indica cuándo ocurrió/reportó.

---

# 65. Reinicios y Event ID

El contador actual de eventos puede reiniciarse cuando reinicia el ESP32.

Por ello el timestamp ayuda a distinguir eventos temporalmente.

Más adelante el Event ID puede fortalecerse con:

```text
persistencia
UUID
boot ID
timestamp
```

Estado:

```text
[PLANEADO]
```

---

# 66. Diagnóstico: `ts = 0`

Revisar:

```text
1. ¿Wi-Fi está conectado?
2. ¿Existe IP?
3. ¿hay Internet/DNS?
4. ¿SNTP inició?
5. ¿SNTP_MAX_SERVERS es suficiente?
6. ¿el servidor NTP es accesible?
7. revisar logs TIME_MANAGER
```

---

# 67. Diagnóstico: `ESP_ERR_INVALID_ARG`

Si aparece inmediatamente después de:

```text
Inicializando sincronizacion SNTP
```

y el log menciona:

```text
SNTP_MAX_SERVERS
```

no cambiar de servidor NTP como primera solución.

Revisar `menuconfig`.

---

# 68. Diagnóstico: timeout SNTP

Si aparece:

```text
No se obtuvo hora SNTP dentro del tiempo esperado
```

revisar:

```text
Internet
DNS
servidor NTP
firewall/red
```

pero el firmware puede continuar.

---

# 69. Diagnóstico: hora incorrecta pero timestamp plausible

Revisar si el problema es:

```text
timezone
```

y no SNTP.

El timestamp UTC puede ser correcto aunque la hora impresa localmente parezca incorrecta.

---

# 70. Diagnóstico: hora cambia repentinamente

Una resincronización puede ajustar el reloj absoluto.

Esto no debe afectar timeouts porque utilizan:

```c
esp_timer_get_time();
```

---

# 71. Relación con `mqtt_manager`

`mqtt_manager` debe solicitar la hora mediante:

```c
time_manager_get_timestamp();
```

No debe tener su propia lógica SNTP.

---

# 72. Función antigua eliminada

Durante la integración se sustituyó una función local tipo:

```c
get_timestamp();
```

dentro de `mqtt_manager`.

La responsabilidad pasó a:

```text
time_manager
```

para mantener separación de responsabilidades.

---

# 73. Relación con `transaction_manager`

`transaction_manager` no debe utilizar:

```c
time_manager_get_timestamp();
```

para calcular expiraciones.

Debe usar:

```c
esp_timer_get_time();
```

---

# 74. Tabla de uso temporal

| Necesidad            | Fuente                 |
| -------------------- | ---------------------- |
| Timestamp MQTT       | SNTP / reloj sistema   |
| Fecha de evento      | SNTP                   |
| Telemetría histórica | SNTP                   |
| ACK Timeout          | `esp_timer_get_time()` |
| Execution Timeout    | `esp_timer_get_time()` |
| Delay FreeRTOS       | ticks                  |
| Debounce futuro      | reloj monotónico/ticks |
| Hora visible         | reloj + timezone       |

---

# 75. Seguridad futura

Una hora confiable será importante para:

```text
TLS
certificados
validación de expiración
firmware firmado
logs de seguridad
```

Por ello `time_manager` será también una dependencia relevante cuando se implemente seguridad de producción.

---

# 76. TLS y hora

Los certificados X.509 dependen de periodos de validez.

Un dispositivo con fecha incorrecta podría rechazar un certificado válido o aceptar comportamientos no deseados.

Por ello, al migrar a:

```text
MQTTS / TLS
```

la estrategia de tiempo deberá revisarse nuevamente.

---

# 77. Posible problema circular futuro

Con TLS puede surgir un problema conceptual:

```text
necesito Internet seguro para SNTP
pero
necesito hora correcta para validar certificados
```

La arquitectura de producción deberá definir cómo establecer una referencia inicial confiable.

Actualmente no aplica al laboratorio MQTT 1883 sin TLS.

---

# 78. Configuración actual resumida

```text
Wi-Fi conectado
      ↓
pool.ntp.org
      ↓
SNTP
      ↓
reloj ESP32
      ↓
Unix UTC
      ↓
MQTT ts
```

---

# 79. Comandos de consulta rápida

## Abrir configuración

```powershell
idf.py menuconfig
```

## Buscar SNTP

```text
/
SNTP_MAX_SERVERS
```

## Compilar

```powershell
idf.py build
```

## Flash + monitor

```powershell
idf.py -p COM6 flash monitor
```

## Escuchar telemetry

```bash
mosquitto_sub -h localhost -p 1883 -u <MQTT_USER> -P '<MQTT_PASSWORD>' -t "maim/v1/devices/<DEVICE_ID>/telemetry" -v
```

---

# 80. Problemas encontrados

| Problema                             | Diagnóstico                     | Solución                    |
| ------------------------------------ | ------------------------------- | --------------------------- |
| `ts = 0`                             | Hora no sincronizada            | Implementar SNTP            |
| `Tried to configure more servers...` | Límite lwIP                     | Aumentar `SNTP_MAX_SERVERS` |
| `ESP_ERR_INVALID_ARG`                | Configuración SNTP incompatible | Corregir `menuconfig`       |
| SNTP falla pero MQTT sigue           | Arquitectura desacoplada        | Comportamiento esperado     |
| Hora local puede diferir             | Timezone                        | Mantener `ts` en UTC        |
| Timeout no debe depender de hora     | Reloj absoluto ajustable        | Usar `esp_timer_get_time()` |

---

# 81. Estado actual

```text
[PROBADO] SNTP inicializa
[PROBADO] sincronización después de Wi-Fi
[PROBADO] Unix timestamp
[PROBADO] timestamp en telemetry
[PROBADO] timestamp en state/reported
[PROBADO] timestamp en events
[PROBADO] timestamp en command/response
[PROBADO] continuidad del firmware si SNTP falla
[PROBADO] timeouts independientes mediante reloj monotónico

[PLANEADO] reintento SNTP en background
[PLANEADO] política de resincronización
[PLANEADO] optimización para batería
[PLANEADO] integración con TLS
```

---

# 82. Relación con otros documentos

Wi-Fi:

```text
06_WIFI.md
```

MQTT:

```text
08_PROTOCOLO_MQTT.md
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

# 83. Resumen

MAIM Connectivity utiliza dos sistemas temporales claramente separados:

```text
SNTP / Unix UTC
        │
        └── fecha y hora absoluta
            para MQTT e históricos

esp_timer_get_time()
        │
        └── tiempo monotónico
            para lógica interna y timeouts
```

Esta separación evita que correcciones del reloj de calendario alteren la operación interna del firmware y permite mantener timestamps consistentes para toda la infraestructura MAIM.

La implementación actual está validada y constituye la referencia temporal base de **MAIM Connectivity v0.1.0**.
