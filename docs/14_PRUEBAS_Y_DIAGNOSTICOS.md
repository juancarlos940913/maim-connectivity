# MAIM Connectivity

## Pruebas y Diagnóstico

**Documento:** `14_PRUEBAS_Y_DIAGNOSTICO.md`
**Proyecto:** MAIM Connectivity
**Objetivo:** Guía rápida de operación, prueba y diagnóstico
**Estado:** [PROBADO] Procedimientos utilizados durante desarrollo

---

# 1. Propósito

Este documento reúne los **comandos, pruebas y resultados esperados** necesarios para comprobar rápidamente el funcionamiento de MAIM Connectivity.

No describe la arquitectura interna del firmware.

Para detalles de implementación consultar los documentos específicos del proyecto.

Esta guía responde principalmente a:

```text
¿Qué comando debo ejecutar?

¿Dónde debo ejecutarlo?

¿Qué debo recibir?

¿Dónde debo verlo?

¿Qué reviso si no funciona?
```

---

# 2. Datos utilizados en laboratorio

Servidor MQTT:

```text
192.168.1.85
```

Puerto:

```text
1883
```

Dispositivo de pruebas:

```text
MM_TEST_001
```

Topic raíz:

```text
maim/v1/devices/MM_TEST_001
```

Puerto ESP32 utilizado durante pruebas:

```text
COM6
```

Los siguientes valores deben sustituirse cuando corresponda:

```text
<MQTT_HOST>
<MQTT_USER>
<MQTT_PASSWORD>
<DEVICE_ID>
<PORT>
```

**Nunca almacenar contraseñas reales dentro de este documento.**

---

# 3. Terminales recomendadas

Durante una prueba completa conviene mantener tres terminales abiertas.

```text
TERMINAL 1
Rocky Linux / SSH
→ escuchar MQTT

TERMINAL 2
Windows PowerShell
→ publicar comandos MQTT

TERMINAL 3
ESP-IDF Monitor
→ observar ESP32 y simular UART
```

---

# 4. Conectarse al servidor Rocky Linux

Desde PowerShell:

```powershell
ssh <USUARIO>@192.168.1.85
```

Resultado esperado:

```text
[usuario@maim-iot-lab ~]$
```

---

# 5. Comprobar IP del servidor

En Rocky Linux:

```bash
hostname -I
```

Debe incluir:

```text
192.168.1.85
```

si esa continúa siendo la IP asignada al laboratorio.

También puede utilizarse:

```bash
ip addr
```

---

# 6. Comprobar Internet del servidor

```bash
ping -c 4 8.8.8.8
```

Resultado correcto:

```text
0% packet loss
```

---

# 7. Comprobar DNS

```bash
ping -c 4 google.com
```

Si:

```bash
ping -c 4 8.8.8.8
```

funciona pero:

```bash
ping -c 4 google.com
```

no funciona, revisar DNS.

---

# 8. Comprobar Mosquitto

```bash
sudo systemctl status mosquitto
```

Debe aparecer:

```text
Active: active (running)
```

Para salir:

```text
q
```

---

# 9. Reiniciar Mosquitto

```bash
sudo systemctl restart mosquitto
```

Después comprobar:

```bash
sudo systemctl status mosquitto
```

---

# 10. Ver logs de Mosquitto

```bash
sudo journalctl -u mosquitto -f
```

Salir:

```text
Ctrl + C
```

---

# 11. Comprobar puerto MQTT

```bash
sudo ss -lntp | grep 1883
```

Debe aparecer un listener sobre:

```text
1883
```

---

# 12. Comprobar firewall

```bash
sudo firewall-cmd --list-ports
```

Debe incluir:

```text
1883/tcp
```

Si no existe:

```bash
sudo firewall-cmd --permanent --add-port=1883/tcp
sudo firewall-cmd --reload
```

---

# 13. Escuchar todo MAIM

En Rocky Linux:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/#" \
-v
```

Este es el comando recomendado para pruebas generales.

---

# 14. Escuchar únicamente el dispositivo de pruebas

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/MM_TEST_001/#" \
-v
```

---

# 15. Escuchar availability

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/MM_TEST_001/availability" \
-v
```

---

# 16. Escuchar telemetría

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/MM_TEST_001/telemetry" \
-v
```

---

# 17. Escuchar estado

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/MM_TEST_001/state/reported" \
-v
```

---

# 18. Escuchar eventos

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/MM_TEST_001/event" \
-v
```

---

# 19. Escuchar respuestas de comandos

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/MM_TEST_001/command/response" \
-v
```

---

# 20. Abrir PowerShell en Mosquitto Windows

Ubicación habitual:

```text
C:\Program Files\mosquitto
```

Desde Explorador de Windows:

```text
1. Abrir la carpeta.
2. Seleccionar la barra de dirección.
3. Escribir powershell.
4. Enter.
```

Después comprobar:

```powershell
.\mosquitto_pub.exe --help
```

o:

```powershell
.\mosquitto_sub.exe --help
```

---

# 21. Prueba Windows → Mosquitto

Rocky escuchando:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/test" \
-v
```

Windows publica:

```powershell
.\mosquitto_pub.exe -h 192.168.1.85 -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/test" -m "Prueba Windows"
```

Rocky debe mostrar:

```text
maim/test Prueba Windows
```

---

# 22. Prueba Rocky → Windows

Windows escucha:

```powershell
.\mosquitto_sub.exe -h 192.168.1.85 -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/test" -v
```

Rocky publica:

```bash
mosquitto_pub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/test" \
-m "Prueba Rocky"
```

Windows debe mostrar:

```text
maim/test Prueba Rocky
```

---

# 23. Verificar que autenticación sea obligatoria

Ejecutar sin credenciales:

```bash
mosquitto_pub \
-h localhost \
-p 1883 \
-t "maim/test" \
-m "Prueba sin login"
```

La publicación debe ser rechazada.

Si funciona, revisar:

```text
allow_anonymous false
```

en la configuración de Mosquitto.

---

# 24. Activar terminal ESP-IDF

Desde PowerShell:

```powershell
& 'C:\Espressif\tools\Microsoft.v5.5.5.PowerShell_profile.ps1'
```

Debe aparecer:

```text
IDF PowerShell Environment
```

y el prompt:

```text
(venv) PS ...
```

---

# 25. Comprobar ESP-IDF

```powershell
idf.py --version
```

Resultado esperado:

```text
ESP-IDF v5.5.5
```

---

# 26. Compilar proyecto

Desde:

```text
C:\MAIM\ESP32\maim_connectivity
```

ejecutar:

```powershell
idf.py build
```

Resultado esperado:

```text
Project build complete.
```

---

# 27. Importante antes de compilar

Guardar todos los archivos:

```text
Ctrl + Shift + S
```

o utilizar:

```text
Save All
```

Si los archivos no están guardados, ESP-IDF compilará la versión anterior almacenada en disco.

---

# 28. Flashear ESP32

```powershell
idf.py -p COM6 flash
```

Resultado esperado al final:

```text
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
```

---

# 29. Flash + Monitor

```powershell
idf.py -p COM6 flash monitor
```

---

# 30. Abrir solamente monitor

```powershell
idf.py -p COM6 monitor
```

---

# 31. Salir del monitor ESP-IDF

Método recomendado:

```text
Ctrl + T
```

soltar.

Después:

```text
X
```

Esto regresa a PowerShell.

---

# 32. Arranque correcto del ESP32

En monitor deben aparecer mensajes similares a:

```text
MAIM: MAIM CONNECTIVITY
MAIM: Device ID : MM_TEST_001
MAIM: Model     : MAIM_MINI
MAIM: HW Rev    : 1.0
MAIM: NVS inicializada correctamente
```

---

# 33. Wi-Fi correcto

Debe aparecer algo similar a:

```text
WIFI_MANAGER: Inicializando WiFi
WIFI_MANAGER: Intentando conectar a: <SSID>
WIFI_MANAGER: WiFi conectado correctamente
WIFI_MANAGER: IP: 192.168.1.x
```

---

# 34. RSSI correcto

Ejemplo:

```text
RSSI: -54 dBm
```

Valores alrededor de:

```text
-45 a -65 dBm
```

son adecuados para las pruebas de laboratorio.

---

# 35. Sincronización de hora correcta

Debe aparecer el inicio de SNTP y posteriormente una hora válida.

La comprobación más sencilla es observar MQTT.

Un timestamp correcto debe ser diferente de:

```text
0
```

---

# 36. Verificar timestamp mediante telemetría

Rocky escuchando:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/MM_TEST_001/telemetry" \
-v
```

Resultado esperado:

```text
maim/v1/devices/MM_TEST_001/telemetry {"schema":1,"ts":1786...,"network":{"rssi_dbm":-54}}
```

Comprobar:

```text
ts != 0
```

---

# 37. MQTT correcto en ESP32

Monitor debe mostrar:

```text
MQTT_MANAGER: Cliente MQTT iniciado
MQTT_MANAGER: MQTT conectado
MQTT_MANAGER: Suscrito a command/request
```

---

# 38. Estado general periódico

Ejemplo:

```text
MAIM: WiFi OK | IP: 192.168.1.66 | RSSI: -54 dBm | MQTT: ONLINE
```

---

# 39. Availability Online

Con Rocky escuchando:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/MM_TEST_001/availability" \
-v
```

reiniciar ESP32.

Debe aparecer un payload con:

```text
"status":"online"
```

---

# 40. Prueba Offline / Last Will

Dejar escuchando:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/MM_TEST_001/availability" \
-v
```

Desconectar físicamente el ESP32 sin cierre MQTT normal.

Durante las pruebas el broker tardó aproximadamente:

```text
~3 minutos
```

en mostrar:

```text
"status":"offline"
```

---

# 41. Probar parser UART

Desde ESP-IDF Monitor escribir:

```text
<STATE,MODE,STANDBY>
```

Resultado esperado:

```text
UART_PROTOCOL: Trama valida | Tipo=STATE | Campos=2
UART_PROTOCOL: Campo[0] = MODE
UART_PROTOCOL: Campo[1] = STANDBY
```

---

# 42. Probar STATE

Enviar:

```text
<STATE,MODE,DISPENSING>
```

Después:

```text
<STATE,BUSY,1>
```

---

# 43. Probar METRIC

Enviar:

```text
<METRIC,WATER_TOTAL_ML,16000>
```

Después:

```text
<METRIC,LAST_DISPENSE_ML,287>
```

Después:

```text
<METRIC,DISPENSE_COUNT,48>
```

---

# 44. Probar SENSOR

Enviar:

```text
<SENSOR,COLD_TANK_C,7.8>
```

---

# 45. Probar OUTPUT

Enviar:

```text
<OUTPUT,VALVE,1>
```

---

# 46. Probar ERROR

Enviar:

```text
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

---

# 47. Probar ERROR_CLEAR

Enviar:

```text
<ERROR_CLEAR,FLOW_SENSOR_FAIL>
```

Después de solicitar estado nuevamente, el error ya no debe aparecer dentro de:

```json
"errors": []
```

---

# 48. Preparar estado completo de prueba

Enviar en ESP-IDF Monitor:

```text
<STATE,MODE,DISPENSING>
```

```text
<STATE,BUSY,1>
```

```text
<METRIC,WATER_TOTAL_ML,16000>
```

```text
<METRIC,LAST_DISPENSE_ML,287>
```

```text
<METRIC,DISPENSE_COUNT,48>
```

```text
<OUTPUT,VALVE,1>
```

```text
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

Después ejecutar `GET_STATE`.

---

# 49. Enviar GET_STATE desde Windows

En PowerShell:

```powershell
.\mosquitto_pub.exe -h 192.168.1.85 -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/MM_TEST_001/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-000101\",\"command\":\"GET_STATE\",\"params\":{}}'
```

---

# 50. Resultado esperado en ESP32

Debe aparecer:

```text
MQTT_MANAGER: GET_STATE recibido
```

y una publicación a:

```text
state/reported
```

---

# 51. Resultado esperado en Mosquitto

Debe observarse algo similar a:

```text
maim/v1/devices/MM_TEST_001/command/request {...}
```

seguido de:

```text
maim/v1/devices/MM_TEST_001/state/reported {...}
```

y:

```text
maim/v1/devices/MM_TEST_001/command/response {...}
```

---

# 52. State esperado

Con los valores anteriores debe contener aproximadamente:

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

---

# 53. Respuesta GET_STATE esperada

```json
{
  "status": "SUCCESS",
  "result": {
    "response": "STATE_PUBLISHED"
  }
}
```

con el mismo:

```text
command_id
```

enviado.

---

# 54. Enviar PING

PowerShell:

```powershell
.\mosquitto_pub.exe -h 192.168.1.85 -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/MM_TEST_001/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-PING-001\",\"command\":\"PING\",\"params\":{}}'
```

---

# 55. Resultado PING

Debe publicarse una respuesta con:

```text
status = SUCCESS
```

y:

```text
response = PONG
```

---

# 56. Probar EVENT DISPENSE_STARTED

En ESP-IDF Monitor:

```text
<EVENT,DISPENSE_STARTED>
```

Mosquitto debe recibir inmediatamente:

```text
type = DISPENSE_STARTED
```

---

# 57. Resultado aproximado

```json
{
  "schema": 1,
  "event_id": "EVT-MM_TEST_001-000001",
  "ts": 1786...,
  "type": "DISPENSE_STARTED",
  "severity": "info",
  "data": {}
}
```

---

# 58. Probar EVENT DISPENSE_COMPLETED

Enviar:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

---

# 59. Resultado esperado

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

# 60. Probar CALIBRATION_COMPLETED

Enviar:

```text
<EVENT,CALIBRATION_COMPLETED,468.2>
```

Resultado esperado:

```json
{
  "type": "CALIBRATION_COMPLETED",
  "data": {
    "pulses_per_liter": 468.2
  }
}
```

Puede existir una pequeña diferencia visual en decimales por representación numérica.

---

# 61. Probar PROGRAMMING_CHANGED

Enviar:

```text
<EVENT,PROGRAMMING_CHANGED,8000>
```

Resultado esperado:

```json
{
  "type": "PROGRAMMING_CHANGED",
  "data": {
    "dispense_time_ms": 8000
  }
}
```

---

# 62. Preparar prueba de comando UART

Mantener una terminal Rocky escuchando:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/MM_TEST_001/#" \
-v
```

Mantener también visible el ESP-IDF Monitor.

---

# 63. Enviar DISPENSE desde Windows

```powershell
.\mosquitto_pub.exe -h 192.168.1.85 -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/MM_TEST_001/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-DISP-001\",\"command\":\"DISPENSE\",\"params\":{}}'
```

---

# 64. Resultado esperado en ESP32

Debe aparecer una trama similar a:

```text
<CMD,1,DISPENSE>
```

El número puede cambiar.

**Utilizar siempre el ID que muestre realmente el ESP32.**

---

# 65. Simular ACK

Si ESP32 mostró:

```text
<CMD,7,DISPENSE>
```

escribir:

```text
<ACK,7>
```

---

# 66. Resultado MQTT después de ACK

Debe aparecer:

```json
{
  "command_id": "CMD-DISP-001",
  "status": "RECEIVED"
}
```

---

# 67. Simular DONE

Después:

```text
<DONE,7>
```

---

# 68. Resultado MQTT después de DONE

Debe aparecer:

```json
{
  "command_id": "CMD-DISP-001",
  "status": "SUCCESS"
}
```

---

# 69. Verificación adicional

Después de `DONE`, esperar más de:

```text
30 segundos
```

No debe aparecer:

```text
EXECUTION_TIMEOUT
```

---

# 70. Probar NACK

Enviar nuevo comando:

```powershell
.\mosquitto_pub.exe -h 192.168.1.85 -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/MM_TEST_001/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-NACK-001\",\"command\":\"DISPENSE\",\"params\":{}}'
```

Observar el nuevo UART ID.

Ejemplo:

```text
<CMD,8,DISPENSE>
```

Responder:

```text
<NACK,8,BUSY>
```

---

# 71. Resultado NACK esperado

```json
{
  "command_id": "CMD-NACK-001",
  "status": "REJECTED",
  "error": {
    "code": "BUSY"
  }
}
```

---

# 72. Prueba ACK Timeout

Enviar:

```powershell
.\mosquitto_pub.exe -h 192.168.1.85 -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/MM_TEST_001/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-TIMEOUT-001\",\"command\":\"DISPENSE\",\"params\":{}}'
```

ESP32 mostrará:

```text
<CMD,ID,DISPENSE>
```

**No escribir ninguna respuesta UART.**

---

# 73. Resultado ACK Timeout

Después de aproximadamente:

```text
5 segundos
```

debe aparecer en MQTT:

```json
{
  "command_id": "CMD-TIMEOUT-001",
  "status": "FAILED",
  "error": {
    "code": "ACK_TIMEOUT"
  }
}
```

---

# 74. Prueba Execution Timeout

Enviar:

```powershell
.\mosquitto_pub.exe -h 192.168.1.85 -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/MM_TEST_001/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-TIMEOUT-002\",\"command\":\"DISPENSE\",\"params\":{}}'
```

Observar UART ID.

Ejemplo:

```text
<CMD,10,DISPENSE>
```

Responder:

```text
<ACK,10>
```

Debe aparecer:

```text
RECEIVED
```

No enviar `DONE`.

---

# 75. Resultado Execution Timeout

Después de aproximadamente:

```text
30 segundos
```

debe aparecer:

```json
{
  "command_id": "CMD-TIMEOUT-002",
  "status": "FAILED",
  "error": {
    "code": "EXECUTION_TIMEOUT"
  }
}
```

---

# 76. Probar respuesta tardía

Generar primero un ACK Timeout.

Después de que MQTT ya haya mostrado:

```text
ACK_TIMEOUT
```

enviar:

```text
<ACK,ID_ANTERIOR>
```

No debe generarse:

```text
RECEIVED
```

para la transacción ya terminada.

---

# 77. Probar DONE desconocido

Enviar directamente:

```text
<DONE,999>
```

sin que exista esa transacción.

Debe registrarse como respuesta desconocida o ignorarse.

No debe publicarse:

```text
SUCCESS
```

para ninguna solicitud activa.

---

# 78. Probar NACK desconocido

Enviar:

```text
<NACK,999,BUSY>
```

No debe afectar una transacción diferente.

---

# 79. Prueba de ERROR_CLEAR completa

Enviar:

```text
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

Ejecutar `GET_STATE`.

Debe aparecer:

```text
FLOW_SENSOR_FAIL
```

Después enviar:

```text
<ERROR_CLEAR,FLOW_SENSOR_FAIL>
```

Ejecutar nuevamente `GET_STATE`.

Debe desaparecer.

---

# 80. Prueba de actualización de valores

Enviar:

```text
<METRIC,WATER_TOTAL_ML,1000>
```

Después:

```text
<METRIC,WATER_TOTAL_ML,2000>
```

Ejecutar:

```text
GET_STATE
```

Debe aparecer:

```json
"water_total_ml": 2000
```

y no dos valores diferentes.

---

# 81. Prueba de booleanos

Enviar:

```text
<OUTPUT,VALVE,1>
```

`GET_STATE` debe mostrar:

```json
"valve": true
```

Después:

```text
<OUTPUT,VALVE,0>
```

debe mostrar:

```json
"valve": false
```

---

# 82. Prueba de modo

Enviar:

```text
<STATE,MODE,STANDBY>
```

`GET_STATE`:

```json
"mode": "STANDBY"
```

Después:

```text
<STATE,MODE,DISPENSING>
```

Debe cambiar a:

```json
"mode": "DISPENSING"
```

---

# 83. Comprobar timestamp desde PowerShell

Si se recibe:

```text
1786497569
```

puede convertirse con:

```powershell
[DateTimeOffset]::FromUnixTimeSeconds(1786497569)
```

Para hora local:

```powershell
[DateTimeOffset]::FromUnixTimeSeconds(1786497569).ToLocalTime()
```

---

# 84. Comprobar timestamp desde Rocky

```bash
date -d @1786497569
```

---

# 85. Comprobar Flash configurada

En ESP-IDF:

```powershell
findstr FLASHSIZE sdkconfig
```

Debe mostrar configuración de:

```text
4MB
```

---

# 86. Comprobar particiones en boot

Durante arranque deben aparecer:

```text
nvs
otadata
phy_init
ota_0
ota_1
avr_fw
```

y:

```text
SPI Flash Size : 4MB
```

---

# 87. Limpiar proyecto

Si existen errores extraños de compilación:

```powershell
idf.py fullclean
```

Después:

```powershell
idf.py build
```

---

# 88. Si `idf.py` no existe

Síntoma:

```text
idf.py no se reconoce...
```

Activar:

```powershell
& 'C:\Espressif\tools\Microsoft.v5.5.5.PowerShell_profile.ps1'
```

Después:

```powershell
idf.py --version
```

---

# 89. Si el código nuevo no aparece

Comprobar primero:

```text
¿El archivo está guardado?
```

Después:

```powershell
idf.py build
idf.py -p COM6 flash monitor
```

---

# 90. Si el ESP32 no conecta a Wi-Fi

Revisar monitor.

Buscar:

```text
WIFI_MANAGER
```

Comprobar:

```text
SSID
contraseña
red 2.4 GHz
RSSI
```

---

# 91. Si Wi-Fi conecta pero MQTT no

Comprobar en ESP32:

```text
IP válida
```

Después en Rocky:

```bash
sudo systemctl status mosquitto
```

```bash
sudo ss -lntp | grep 1883
```

```bash
sudo firewall-cmd --list-ports
```

---

# 92. Si MQTT dice `Connection refused`

Revisar:

```text
IP del servidor
Mosquitto activo
puerto 1883
firewall
```

---

# 93. Si MQTT dice `Not authorized`

Revisar:

```text
MQTT_USER
MQTT_PASSWORD
/etc/mosquitto/passwd
```

En Rocky:

```bash
ls -l /etc/mosquitto/passwd
```

---

# 94. Si `mosquitto_sub` muestra error `-h`

Error:

```text
Error: -h argument given but no host specified.
```

Correcto:

```text
-h 192.168.1.85
```

Nunca dejar:

```text
-h
```

sin dirección inmediatamente después.

---

# 95. Si telemetría muestra `ts:0`

Revisar monitor ESP32.

Buscar:

```text
TIME_MANAGER
```

Si aparece:

```text
SNTP_MAX_SERVERS
```

revisar:

```powershell
idf.py menuconfig
```

---

# 96. Si SNTP falla pero MQTT funciona

Esto puede ocurrir.

Resultado:

```text
MQTT ONLINE
```

pero:

```text
ts = 0
```

El problema debe buscarse en SNTP/Internet/DNS y no necesariamente en Mosquitto.

---

# 97. Si `state/reported` no muestra un dato

Comprobar primero que la trama UART correspondiente haya sido enviada.

Ejemplo:

```text
<METRIC,WATER_TOTAL_ML,16000>
```

Después ejecutar:

```text
GET_STATE
```

---

# 98. Si un evento no aparece

Primero escuchar exactamente:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/MM_TEST_001/event" \
-v
```

Después enviar:

```text
<EVENT,DISPENSE_STARTED>
```

---

# 99. Si ACK no produce RECEIVED

Comprobar que se está utilizando **exactamente el mismo UART ID**.

Ejemplo:

```text
CMD:
<CMD,17,DISPENSE>
```

correcto:

```text
<ACK,17>
```

incorrecto:

```text
<ACK,16>
```

---

# 100. Si DONE no produce SUCCESS

Comprobar:

```text
1. El ID es correcto.
2. La transacción recibió ACK antes.
3. No expiró ya por timeout.
```

---

# 101. Si aparece ACK_TIMEOUT aunque mandaste ACK

Comprobar:

```text
ID correcto
ACK antes de ~5 s
trama completa
<
>
```

Correcto:

```text
<ACK,15>
```

---

# 102. Si aparece EXECUTION_TIMEOUT aunque mandaste DONE

Comprobar:

```text
DONE antes de ~30 s después del ACK
ID correcto
transacción todavía activa
```

Correcto:

```text
<DONE,15>
```

---

# 103. Secuencia de prueba completa recomendada

Después de cualquier cambio importante de firmware:

```text
1. Build.
2. Flash.
3. Boot correcto.
4. Wi-Fi conectado.
5. Timestamp válido.
6. MQTT conectado.
7. Availability online.
8. Telemetry recibida.
9. STATE manual.
10. METRIC manual.
11. OUTPUT manual.
12. ERROR manual.
13. GET_STATE.
14. EVENT.
15. DISPENSE.
16. ACK.
17. DONE.
18. NACK.
19. ACK Timeout.
20. Execution Timeout.
```

---

# 104. Smoke Test mínimo

Si solo se necesita comprobar rápidamente que el sistema básico está vivo:

```text
1. Encender ESP32.
2. Confirmar WiFi OK.
3. Confirmar MQTT ONLINE.
4. Escuchar maim/v1/devices/MM_TEST_001/#.
5. Confirmar telemetry.
6. Enviar PING.
7. Confirmar PONG.
8. Enviar GET_STATE.
9. Confirmar state/reported.
```

Si todo esto funciona, la conectividad principal está operativa.

---

# 105. Smoke Test UART

Enviar:

```text
<STATE,MODE,STANDBY>
```

Después:

```text
<METRIC,WATER_TOTAL_ML,12345>
```

Después:

```text
<OUTPUT,VALVE,0>
```

Ejecutar `GET_STATE`.

Debe reflejar:

```text
STANDBY
12345
false
```

---

# 106. Smoke Test de eventos

Enviar:

```text
<EVENT,DISPENSE_STARTED>
```

Debe aparecer inmediatamente en:

```text
/event
```

---

# 107. Smoke Test de transacciones

Enviar MQTT:

```text
DISPENSE
```

Observar:

```text
<CMD,ID,DISPENSE>
```

Responder:

```text
<ACK,ID>
```

Debe aparecer:

```text
RECEIVED
```

Después:

```text
<DONE,ID>
```

Debe aparecer:

```text
SUCCESS
```

---

# 108. Diagnóstico por capas

Cuando algo falla, probar en este orden:

```text
1. Rocky Linux
2. Mosquitto
3. Windows ↔ Mosquitto
4. ESP32 Wi-Fi
5. ESP32 MQTT
6. Telemetry
7. MQTT command/request
8. UART parser
9. State / Event
10. Transaction responses
```

Esto ayuda a localizar la capa exacta del problema.

---

# 109. Regla práctica

Si Windows y Rocky pueden intercambiar:

```text
maim/test
```

pero ESP32 no conecta:

```text
problema probable:
ESP32 / WiFi / credenciales MQTT
```

---

# 110. Otra regla práctica

Si ESP32 publica telemetría pero no responde a:

```text
GET_STATE
```

el broker y Wi-Fi ya funcionan.

Buscar el problema en:

```text
command/request
JSON
mqtt_manager
```

---

# 111. Otra regla práctica

Si `GET_STATE` funciona pero un:

```text
EVENT
```

manual no aparece:

buscar en:

```text
UART EVENT
callback
publicación event
```

---

# 112. Otra regla práctica

Si MQTT genera:

```text
<CMD,ID,DISPENSE>
```

pero ACK no produce `RECEIVED`:

el camino:

```text
MQTT → ESP32 → UART
```

ya funciona.

Buscar en:

```text
ACK
transaction ID
respuesta UART
```

---

# 113. Otra regla práctica

Si ACK genera `RECEIVED` pero DONE no genera `SUCCESS`:

buscar en:

```text
DONE ID
estado de transacción
timeout
```

---

# 114. Comandos rápidos Rocky Linux

Estado Mosquitto:

```bash
sudo systemctl status mosquitto
```

Reiniciar:

```bash
sudo systemctl restart mosquitto
```

Logs:

```bash
sudo journalctl -u mosquitto -f
```

Puerto:

```bash
sudo ss -lntp | grep 1883
```

IP:

```bash
hostname -I
```

Firewall:

```bash
sudo firewall-cmd --list-ports
```

---

# 115. Comandos rápidos ESP-IDF

Versión:

```powershell
idf.py --version
```

Build:

```powershell
idf.py build
```

Limpiar:

```powershell
idf.py fullclean
```

Flash:

```powershell
idf.py -p COM6 flash
```

Monitor:

```powershell
idf.py -p COM6 monitor
```

Flash + monitor:

```powershell
idf.py -p COM6 flash monitor
```

---

# 116. Comandos rápidos MQTT

Escuchar todo:

```bash
mosquitto_sub -h localhost -p 1883 -u <MQTT_USER> -P '<MQTT_PASSWORD>' -t "maim/#" -v
```

Escuchar equipo:

```bash
mosquitto_sub -h localhost -p 1883 -u <MQTT_USER> -P '<MQTT_PASSWORD>' -t "maim/v1/devices/MM_TEST_001/#" -v
```

---

# 117. Tramas UART rápidas

Estado:

```text
<STATE,MODE,STANDBY>
```

Busy:

```text
<STATE,BUSY,0>
```

Métrica:

```text
<METRIC,WATER_TOTAL_ML,16000>
```

Último dispensado:

```text
<METRIC,LAST_DISPENSE_ML,287>
```

Conteo:

```text
<METRIC,DISPENSE_COUNT,48>
```

Sensor:

```text
<SENSOR,COLD_TANK_C,7.8>
```

Salida:

```text
<OUTPUT,VALVE,1>
```

Error:

```text
<ERROR,FLOW_SENSOR_FAIL,ERROR>
```

Limpiar error:

```text
<ERROR_CLEAR,FLOW_SENSOR_FAIL>
```

Evento:

```text
<EVENT,DISPENSE_STARTED>
```

Evento completo:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

ACK:

```text
<ACK,ID>
```

DONE:

```text
<DONE,ID>
```

NACK:

```text
<NACK,ID,BUSY>
```

---

# 118. Checklist después de reiniciar todo el laboratorio

Después de apagar y volver a iniciar Rocky + ESP32:

```text
[ ] Rocky inicia.
[ ] IP conocida.
[ ] SSH funciona.
[ ] Mosquitto active.
[ ] Puerto 1883 escuchando.
[ ] ESP32 inicia.
[ ] WiFi conectado.
[ ] IP ESP32 válida.
[ ] Hora sincronizada.
[ ] MQTT conectado.
[ ] Availability online.
[ ] Telemetry recibida.
[ ] PING → PONG.
[ ] GET_STATE funciona.
```

---

# 119. Checklist antes de considerar firmware estable

```text
[ ] Build sin errores.
[ ] Flash sin errores.
[ ] Boot sin crash.
[ ] WiFi estable.
[ ] SNTP correcto.
[ ] MQTT estable.
[ ] Telemetry correcta.
[ ] state/reported correcto.
[ ] Eventos correctos.
[ ] ACK correcto.
[ ] DONE correcto.
[ ] NACK correcto.
[ ] ACK Timeout correcto.
[ ] Execution Timeout correcto.
[ ] Ningún timeout después de SUCCESS.
```

---

# 120. Estado de pruebas actual

```text
[PROBADO] Rocky Linux
[PROBADO] SSH
[PROBADO] Mosquitto
[PROBADO] autenticación MQTT
[PROBADO] Windows → MQTT
[PROBADO] MQTT → Windows
[PROBADO] ESP32 Wi-Fi
[PROBADO] ESP32 MQTT
[PROBADO] SNTP
[PROBADO] telemetry
[PROBADO] availability
[PROBADO] Last Will
[PROBADO] UART manual
[PROBADO] STATE
[PROBADO] METRIC
[PROBADO] SENSOR
[PROBADO] OUTPUT
[PROBADO] ERROR
[PROBADO] GET_STATE
[PROBADO] EVENT
[PROBADO] MQTT → CMD UART
[PROBADO] ACK
[PROBADO] DONE
[PROBADO] NACK
[PROBADO] ACK_TIMEOUT
[PROBADO] EXECUTION_TIMEOUT
```

---

# 121. Limitación actual de las pruebas

Actualmente la comunicación con el controlador principal está siendo simulada escribiendo manualmente tramas desde:

```text
ESP-IDF Monitor
```

Por tanto, todavía falta repetir las pruebas de UART con:

```text
ATmega físico
↕
UART físico
↕
ESP32
```

cuando la placa correspondiente esté disponible.

---

# 122. Prueba siguiente con hardware físico

Cuando exista la placa:

```text
1. Mantener las mismas tramas.
2. Sustituir terminal manual por UART físico.
3. Repetir STATE.
4. Repetir METRIC.
5. Repetir EVENT.
6. Repetir CMD.
7. Repetir ACK/DONE/NACK.
8. Repetir timeouts.
9. Forzar reinicio ATmega.
10. Forzar desconexión UART.
```

Si estas pruebas producen los mismos resultados MQTT, la integración podrá considerarse equivalente a la simulación actual.

---

# 123. Regla de uso de esta guía

Cuando aparezca una falla:

```text
NO modificar varias cosas a la vez.
```

Primero localizar la capa mediante las pruebas más simples.

Ejemplo:

```text
¿Mosquitto funciona?
↓
¿ESP32 publica?
↓
¿ESP32 recibe?
↓
¿UART recibe?
↓
¿transacción responde?
```

Esto reduce considerablemente el tiempo de diagnóstico.

---

# 124. Resumen operativo

Para comprobar rápidamente MAIM Connectivity:

```text
Rocky:
mosquitto_sub ... -t "maim/v1/devices/MM_TEST_001/#" -v
```

```text
ESP32:
idf.py -p COM6 monitor
```

```text
Windows:
mosquitto_pub → GET_STATE / DISPENSE
```

y utilizar el monitor ESP32 para simular:

```text
<STATE,...>
<EVENT,...>
<ACK,...>
<DONE,...>
<NACK,...>
```

Los resultados deben aparecer en Mosquitto mediante:

```text
availability
telemetry
state/reported
event
command/response
```

Esta guía constituye la referencia rápida para **probar, verificar y diagnosticar el entorno MAIM Connectivity sin necesidad de revisar la implementación interna de cada módulo**.

---

**Fin del documento — `14_PRUEBAS_Y_DIAGNOSTICO.md`**
