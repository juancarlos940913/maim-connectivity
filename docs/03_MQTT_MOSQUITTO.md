# MAIM Connectivity
# MQTT y Mosquitto

**Documento:** 03_MQTT_MOSQUITTO.md

**Proyecto:** MAIM Connectivity

**Broker:** Eclipse Mosquitto

**Protocolo:** MQTT v3.1.1 / v5 compatible

**Puerto de laboratorio:** 1883/TCP

**Servidor de laboratorio:** Rocky Linux 9.8 Minimal

**Estado:** [PROBADO] Broker funcional y validado con ESP32


---

# 1. Propósito

Este documento describe la instalación, configuración, operación y diagnóstico del broker MQTT utilizado por **MAIM Connectivity**.

El broker actual es:

```text
Eclipse Mosquitto
```

y funciona dentro del servidor Rocky Linux de laboratorio.

La arquitectura general es:

```text
ESP32
  │
  │ MQTT
  ▼
Mosquitto
  │
  ├── Windows PowerShell
  ├── Backend futuro
  └── Servicios MAIM
```

Mosquitto actúa como intermediario de mensajes entre dispositivos y servicios.

---

# 2. Arquitectura MQTT del laboratorio

```text
               WINDOWS
                  │
          mosquitto_pub/sub
                  │
                  ▼
        ┌──────────────────┐
        │ Rocky Linux 9.8  │
        │                  │
        │    Mosquitto     │
        │    TCP 1883      │
        └────────┬─────────┘
                 │
                 │ LAN
                 ▼
              ESP32
```

Durante el desarrollo se utilizó:

```text
Broker IP:
192.168.1.85

Puerto:
1883
```

Estos valores corresponden únicamente al entorno de laboratorio.

---

# 3. Función de MQTT dentro de MAIM

MQTT se utiliza para transportar:

```text
availability
telemetry
state/reported
event
command/request
command/response
```

La raíz actual del protocolo es:

```text
maim/v1/devices
```

Ejemplo:

```text
maim/v1/devices/MM_TEST_001/telemetry
```

La especificación completa del protocolo se encuentra en:

```text
08_PROTOCOLO_MQTT.md
```

---

# 4. Instalación de Mosquitto

En Rocky Linux Minimal puede ocurrir que el paquete:

```text
mosquitto
```

no se encuentre disponible directamente con los repositorios habilitados inicialmente.

Durante el desarrollo apareció:

```text
No match for argument: mosquitto
```

Esto significa que el paquete no estaba disponible en los repositorios activos.

Debe habilitarse el repositorio adecuado para Rocky Linux antes de instalarlo.

Una vez disponible:

```bash
sudo dnf install mosquitto -y
```

Comprobar versión:

```bash
mosquitto -h
```

La versión utilizada durante el desarrollo fue:

```text
Mosquitto 2.0.22
```

---

# 5. Habilitar el servicio

Después de instalar:

```bash
sudo systemctl enable --now mosquitto
```

Esto:

```text
enable
```

configura el arranque automático.

Y:

```text
--now
```

inicia el servicio inmediatamente.

---

# 6. Comprobar estado

```bash
sudo systemctl status mosquitto
```

Resultado esperado:

```text
Active: active (running)
```

Si aparece:

```text
lines 1-26/26 (END)
```

el comando está abierto dentro del pager.

Salir presionando:

```text
q
```

---

# 7. Comandos de servicio

## Iniciar

```bash
sudo systemctl start mosquitto
```

## Detener

```bash
sudo systemctl stop mosquitto
```

## Reiniciar

```bash
sudo systemctl restart mosquitto
```

## Estado

```bash
sudo systemctl status mosquitto
```

## Habilitar en arranque

```bash
sudo systemctl enable mosquitto
```

---

# 8. Logs

Consultar logs:

```bash
sudo journalctl -u mosquitto
```

Seguir logs en tiempo real:

```bash
sudo journalctl -u mosquitto -f
```

Este comando es especialmente útil para diagnosticar:

* conexiones;
* desconexiones;
* autenticación;
* errores de configuración;
* Last Will;
* clientes MQTT.

---

# 9. Configuración principal

El archivo principal normalmente es:

```text
/etc/mosquitto/mosquitto.conf
```

Durante el desarrollo se decidió utilizar archivos de configuración separados dentro de:

```text
/etc/mosquitto/conf.d/
```

Esto permite mantener configuraciones MAIM separadas del archivo principal.

---

# 10. Problema: `conf.d` no existe

Al intentar crear:

```bash
sudo nano /etc/mosquitto/conf.d/maim.conf
```

apareció:

```text
No such file or directory
```

El problema era que:

```text
/etc/mosquitto/conf.d/
```

todavía no existía.

Crear:

```bash
sudo mkdir -p /etc/mosquitto/conf.d
```

Después:

```bash
sudo nano /etc/mosquitto/conf.d/maim.conf
```

---

# 11. Habilitar `include_dir`

Mosquitto solo cargará los archivos de `conf.d` si el archivo principal incluye esa carpeta.

Durante la revisión del archivo se encontró:

```text
# include_dir option
...
#include_dir
```

La configuración estaba comentada.

Editar:

```bash
sudo nano /etc/mosquitto/mosquitto.conf
```

Agregar o descomentar:

```text
include_dir /etc/mosquitto/conf.d
```

Guardar.

---

# 12. Configuración MAIM

El archivo utilizado es:

```text
/etc/mosquitto/conf.d/maim.conf
```

Configuración base:

```text
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd
```

Esto significa:

```text
listener 1883
→ escuchar MQTT en TCP 1883

allow_anonymous false
→ no permitir clientes sin autenticación

password_file
→ utilizar archivo de usuarios/contraseñas
```

---

# 13. Crear usuario MQTT

Crear archivo de contraseñas:

```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd <MQTT_USER>
```

El comando solicitará:

```text
Password:
Reenter password:
```

No documentar la contraseña real.

Para agregar otro usuario posteriormente sin sobrescribir:

```bash
sudo mosquitto_passwd /etc/mosquitto/passwd <NUEVO_USUARIO>
```

No utilizar `-c` si el archivo ya existe y se quieren conservar los usuarios anteriores.

---

# 14. Permisos del archivo passwd

Durante la validación apareció:

```text
Warning: File /etc/mosquitto/passwd owner is not mosquitto.
Future versions will refuse to load this file.
```

La solución es asignar el archivo al usuario del servicio.

```bash
sudo chown mosquitto:mosquitto /etc/mosquitto/passwd
```

También puede limitarse acceso:

```bash
sudo chmod 600 /etc/mosquitto/passwd
```

Comprobar:

```bash
ls -l /etc/mosquitto/passwd
```

---

# 15. Problema: archivo passwd inexistente

Al validar inicialmente Mosquitto apareció:

```text
Error: Unable to open pwfile "/etc/mosquitto/passwd".
Error opening password file "/etc/mosquitto/passwd".
```

Esto significa que `maim.conf` ya hacía referencia al archivo, pero este todavía no había sido creado.

La solución fue:

```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd <MQTT_USER>
```

y posteriormente corregir propietario.

---

# 16. Validación de configuración

Durante el desarrollo se intentó:

```bash
mosquitto -t
```

pero Mosquitto respondió:

```text
Error: Unknown option '-t'.
```

En Mosquitto 2.0.22 `-t` no corresponde a una opción de validación de configuración.

La forma utilizada para comprobar la configuración fue iniciar Mosquitto en foreground con configuración explícita y verbose:

```bash
sudo mosquitto -c /etc/mosquitto/mosquitto.conf -v
```

Si la configuración es correcta debe aparecer algo similar a:

```text
Loading config file /etc/mosquitto/conf.d/maim.conf
mosquitto version 2.0.22 starting
Config loaded from /etc/mosquitto/mosquitto.conf.
Opening ipv4 listen socket on port 1883.
Opening ipv6 listen socket on port 1883.
mosquitto version 2.0.22 running
```

Salir con:

```text
Ctrl + C
```

Después usar nuevamente el servicio systemd.

---

# 17. Reiniciar después de cambios

Después de modificar:

```text
mosquitto.conf
maim.conf
passwd
```

ejecutar:

```bash
sudo systemctl restart mosquitto
```

Después:

```bash
sudo systemctl status mosquitto
```

---

# 18. Verificar puerto

Comprobar que existe listener en 1883:

```bash
sudo ss -lntp | grep 1883
```

Resultado esperado similar a:

```text
LISTEN ... 0.0.0.0:1883
```

y posiblemente:

```text
[::]:1883
```

---

# 19. Firewall

Si el ESP32 o Windows no pueden acceder desde la LAN, abrir el puerto:

```bash
sudo firewall-cmd --permanent --add-port=1883/tcp
```

Recargar:

```bash
sudo firewall-cmd --reload
```

Comprobar:

```bash
sudo firewall-cmd --list-ports
```

Debe aparecer:

```text
1883/tcp
```

---

# 20. Primera prueba local sin autenticación

Antes de activar autenticación se utilizó una prueba local tipo:

Terminal 1:

```bash
mosquitto_sub -h localhost -t "maim/test"
```

Terminal 2:

```bash
mosquitto_pub -h localhost -t "maim/test" -m "Prueba"
```

El mensaje debe aparecer inmediatamente en la terminal suscrita.

---

# 21. Prueba después de activar autenticación

Escuchar:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/test" \
-v
```

Publicar:

```bash
mosquitto_pub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/test" \
-m "Prueba MQTT"
```

Resultado:

```text
maim/test Prueba MQTT
```

---

# 22. Comprobar que login es obligatorio

Intentar:

```bash
mosquitto_pub \
-h localhost \
-p 1883 \
-t "maim/test" \
-m "Prueba sin login"
```

Debe rechazarse.

Si funciona sin usuario y contraseña, revisar:

```text
allow_anonymous false
```

---

# 23. Prueba Windows → Rocky

Instalar Mosquitto Client en Windows.

Ubicación habitual:

```text
C:\Program Files\mosquitto
```

Abrir PowerShell en esa carpeta.

Una forma sencilla:

1. abrir la carpeta en Explorador;
2. hacer clic en barra de dirección;
3. escribir:

```text
powershell
```

4. Enter.

---

# 24. Error de `-h`

Durante una prueba apareció:

```text
Error: -h argument given but no host specified.
```

Esto ocurre si se escribe:

```text
-h
```

sin colocar inmediatamente la dirección del broker.

Correcto:

```powershell
.\mosquitto_sub.exe -h 192.168.1.85 ...
```

Incorrecto:

```powershell
.\mosquitto_sub.exe -h -p 1883 ...
```

---

# 25. Escuchar desde Windows

```powershell
.\mosquitto_sub.exe `
-h <MQTT_HOST> `
-p 1883 `
-u <MQTT_USER> `
-P "<MQTT_PASSWORD>" `
-t "maim/#" `
-v
```

También puede escribirse en una sola línea:

```powershell
.\mosquitto_sub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/#" -v
```

---

# 26. Publicar desde Windows

```powershell
.\mosquitto_pub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/test" -m "Mensaje desde Windows"
```

Rocky debe recibirlo si está escuchando:

```bash
mosquitto_sub -h localhost -p 1883 -u <MQTT_USER> -P '<MQTT_PASSWORD>' -t "maim/#" -v
```

---

# 27. Prueba inversa Rocky → Windows

Windows escucha:

```powershell
.\mosquitto_sub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/test" -v
```

Rocky publica:

```bash
mosquitto_pub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/test" \
-m "Mensaje desde Rocky"
```

Windows debe recibir:

```text
maim/test Mensaje desde Rocky
```

---

# 28. Escuchar todos los mensajes MAIM

En Rocky:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/#" \
-v
```

Este comando es uno de los más útiles durante desarrollo.

---

# 29. Escuchar únicamente un dispositivo

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/<DEVICE_ID>/#" \
-v
```

Ejemplo de laboratorio:

```text
DEVICE_ID = MM_TEST_001
```

---

# 30. Escuchar telemetría

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/<DEVICE_ID>/telemetry" \
-v
```

Ejemplo de mensaje:

```text
maim/v1/devices/MM_TEST_001/telemetry {"schema":1,"ts":1786493729,"network":{"rssi_dbm":-54}}
```

---

# 31. Escuchar availability

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/<DEVICE_ID>/availability" \
-v
```

Online:

```json
{
  "schema": 1,
  "ts": 1786493729,
  "status": "online"
}
```

---

# 32. Last Will / Offline

El ESP32 configura un Last Will MQTT.

Si el equipo desaparece abruptamente, el broker termina publicando:

```json
{
  "schema": 1,
  "status": "offline"
}
```

Durante las pruebas con la configuración normal de keepalive se observó un tiempo aproximado de:

```text
~3 minutos
```

antes de que Mosquitto detectara la pérdida del cliente.

Se decidió mantener el keepalive normal y dejar que Mosquitto determine el estado offline según su comportamiento estándar.

---

# 33. Prueba de Last Will

Escuchar:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/<DEVICE_ID>/availability" \
-v
```

Encender ESP32.

Debe aparecer:

```text
status = online
```

Desconectar alimentación abruptamente.

Esperar el tiempo de detección.

Debe aparecer:

```text
status = offline
```

---

# 34. Ver retained availability

Como availability utiliza retain, puede comprobarse:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/<DEVICE_ID>/availability" \
-C 1 \
-v
```

Debe devolver inmediatamente el último estado retenido.

---

# 35. `state/reported`

Escuchar:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/<DEVICE_ID>/state/reported" \
-v
```

Ejemplo validado:

```json
{
  "schema": 1,
  "ts": 1786497569,
  "device": {
    "device_id": "MM_TEST_001",
    "model": "MAIM_MINI",
    "hw_rev": "1.0"
  },
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
  "network": {
    "type": "wifi",
    "connected": true,
    "rssi_dbm": -52,
    "ip": "192.168.1.66"
  },
  "firmware": {
    "esp32": {
      "version": "0.1.0"
    }
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

# 36. Enviar `GET_STATE`

Desde PowerShell Windows:

```powershell
.\mosquitto_pub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/<DEVICE_ID>/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-000101\",\"command\":\"GET_STATE\",\"params\":{}}'
```

El flujo esperado es:

```text
command/request
↓
state/reported
↓
command/response
```

---

# 37. Respuesta a GET_STATE

Ejemplo:

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

# 38. PING

Enviar:

```powershell
.\mosquitto_pub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/<DEVICE_ID>/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-000001\",\"command\":\"PING\",\"params\":{}}'
```

Respuesta:

```json
{
  "schema": 1,
  "command_id": "CMD-000001",
  "status": "SUCCESS",
  "result": {
    "response": "PONG"
  }
}
```

---

# 39. Escuchar eventos

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/<DEVICE_ID>/event" \
-v
```

---

# 40. Eventos validados

Ejemplos recibidos correctamente:

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

```json
{
  "schema": 1,
  "event_id": "EVT-MM_TEST_001-000003",
  "ts": 1786504244,
  "type": "CALIBRATION_COMPLETED",
  "severity": "info",
  "data": {
    "pulses_per_liter": 468.2
  }
}
```

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

---

# 41. Enviar DISPENSE

Desde Windows:

```powershell
.\mosquitto_pub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/<DEVICE_ID>/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-000200\",\"command\":\"DISPENSE\",\"params\":{}}'
```

El ESP32 genera internamente una trama UART tipo:

```text
<CMD,1,DISPENSE>
```

---

# 42. ACK

Cuando el controlador responde:

```text
<ACK,1>
```

MQTT publica:

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "status": "RECEIVED"
}
```

---

# 43. DONE

Cuando llega:

```text
<DONE,1>
```

MQTT publica:

```json
{
  "schema": 1,
  "command_id": "CMD-000200",
  "status": "SUCCESS"
}
```

---

# 44. NACK

Si el controlador responde:

```text
<NACK,2,BUSY>
```

MQTT publica:

```json
{
  "schema": 1,
  "command_id": "CMD-000201",
  "status": "REJECTED",
  "error": {
    "code": "BUSY"
  }
}
```

---

# 45. ACK Timeout

Enviar un comando:

```powershell
.\mosquitto_pub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/<DEVICE_ID>/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-TIMEOUT-001\",\"command\":\"DISPENSE\",\"params\":{}}'
```

No enviar ACK.

Después de aproximadamente:

```text
5 segundos
```

se debe publicar:

```json
{
  "schema": 1,
  "command_id": "CMD-TIMEOUT-001",
  "status": "FAILED",
  "error": {
    "code": "ACK_TIMEOUT"
  }
}
```

---

# 46. Execution Timeout

Enviar:

```powershell
.\mosquitto_pub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/<DEVICE_ID>/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-TIMEOUT-002\",\"command\":\"DISPENSE\",\"params\":{}}'
```

Responder:

```text
<ACK,ID>
```

pero no enviar:

```text
<DONE,ID>
```

Después de aproximadamente:

```text
30 segundos
```

debe publicarse:

```json
{
  "schema": 1,
  "command_id": "CMD-TIMEOUT-002",
  "status": "FAILED",
  "error": {
    "code": "EXECUTION_TIMEOUT"
  }
}
```

---

# 47. Prueba de transacción exitosa

Enviar:

```powershell
.\mosquitto_pub.exe -h <MQTT_HOST> -p 1883 -u <MQTT_USER> -P "<MQTT_PASSWORD>" -t "maim/v1/devices/<DEVICE_ID>/command/request" -m '{\"schema\":1,\"command_id\":\"CMD-TIMEOUT-003\",\"command\":\"DISPENSE\",\"params\":{}}'
```

ESP32 mostrará:

```text
<CMD,ID,DISPENSE>
```

Responder con ese mismo ID:

```text
<ACK,ID>
```

y después:

```text
<DONE,ID>
```

Debe producir:

```text
RECEIVED
↓
SUCCESS
```

y no debe generar timeout después.

---

# 48. Diagnóstico: cliente no conecta

Comprobar:

```bash
sudo systemctl status mosquitto
```

Después:

```bash
sudo ss -lntp | grep 1883
```

Después:

```bash
sudo firewall-cmd --list-ports
```

Después:

```bash
sudo journalctl -u mosquitto -f
```

---

# 49. Diagnóstico: `Connection refused`

Posibles causas:

```text
Mosquitto detenido
puerto incorrecto
listener no configurado
firewall
IP incorrecta
```

Comprobar:

```bash
sudo systemctl restart mosquitto
```

y:

```bash
sudo ss -lntp | grep 1883
```

---

# 50. Diagnóstico: `Not authorized`

Posibles causas:

```text
usuario incorrecto
contraseña incorrecta
allow_anonymous false
archivo passwd incorrecto
permisos de passwd
```

Comprobar:

```bash
ls -l /etc/mosquitto/passwd
```

y:

```bash
sudo journalctl -u mosquitto -f
```

---

# 51. Diagnóstico: no aparecen mensajes

Verificar primero que la suscripción usa el topic correcto.

Para pruebas generales utilizar:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "#" \
-v
```

Esto escucha todos los topics accesibles al usuario.

Si aquí aparecen mensajes pero no en:

```text
maim/#
```

revisar el topic publicado.

---

# 52. Wildcards MQTT útiles

## `#`

Múltiples niveles.

Ejemplo:

```text
maim/#
```

escucha todo bajo `maim`.

## `+`

Un solo nivel.

Ejemplo:

```text
maim/v1/devices/+/availability
```

escucha availability de todos los dispositivos.

---

# 53. Escuchar availability de todos los equipos

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/v1/devices/+/availability" \
-v
```

Esto será útil cuando existan múltiples dispositivos.

---

# 54. QoS actual

La arquitectura inicial utiliza conceptualmente:

```text
availability       QoS 1
state/reported     QoS 1
event              QoS 1
command/request    QoS 1
command/response   QoS 1

telemetry          QoS 0
```

La especificación definitiva se documenta en:

```text
08_PROTOCOLO_MQTT.md
```

---

# 55. Retain actual

Se utiliza retain principalmente para:

```text
availability
state/reported
```

Esto permite que un nuevo suscriptor pueda conocer inmediatamente el último estado reportado.

Los eventos no deben utilizar retain.

Los comandos no deben utilizar retain.

---

# 56. Regla crítica: comandos no retained

Nunca publicar:

```text
command/request
```

con retain.

Un comando retained podría provocar que un dispositivo recién conectado recibiera y ejecutara una operación antigua.

Los comandos deben representar solicitudes nuevas y explícitas.

---

# 57. Seguridad actual

La seguridad actual del laboratorio utiliza:

```text
usuario
+
contraseña
```

sobre:

```text
MQTT TCP 1883
```

Esto es suficiente únicamente para laboratorio controlado.

No debe considerarse configuración final de producción.

---

# 58. Seguridad futura

Antes de desplegar equipos fuera de una red controlada deberán evaluarse:

```text
MQTT TLS
puerto 8883
certificados
credenciales individuales
ACL
rotación de credenciales
revocación
protección del broker
firewall
VPN o infraestructura privada
```

Estado:

```text
[PLANEADO]
```

---

# 59. Configuración futura recomendada

En producción cada dispositivo debería disponer de credenciales individuales.

Ejemplo conceptual:

```text
Device:
MM_00001234

Username:
device_MM_00001234
```

y ACL que permita únicamente:

```text
maim/v1/devices/MM_00001234/#
```

Esto evitaría que un dispositivo pudiera publicar o suscribirse a los topics de otro.

---

# 60. Comandos de consulta rápida

## Estado

```bash
sudo systemctl status mosquitto
```

## Reiniciar

```bash
sudo systemctl restart mosquitto
```

## Logs

```bash
sudo journalctl -u mosquitto -f
```

## Puerto

```bash
sudo ss -lntp | grep 1883
```

## Firewall

```bash
sudo firewall-cmd --list-ports
```

## Escuchar todo MAIM

```bash
mosquitto_sub -h localhost -p 1883 -u <MQTT_USER> -P '<MQTT_PASSWORD>' -t "maim/#" -v
```

## Escuchar dispositivo

```bash
mosquitto_sub -h localhost -p 1883 -u <MQTT_USER> -P '<MQTT_PASSWORD>' -t "maim/v1/devices/<DEVICE_ID>/#" -v
```

## Eventos

```bash
mosquitto_sub -h localhost -p 1883 -u <MQTT_USER> -P '<MQTT_PASSWORD>' -t "maim/v1/devices/<DEVICE_ID>/event" -v
```

## Availability

```bash
mosquitto_sub -h localhost -p 1883 -u <MQTT_USER> -P '<MQTT_PASSWORD>' -t "maim/v1/devices/<DEVICE_ID>/availability" -v
```

---

# 61. Secuencia de verificación después de reiniciar servidor

Después de reiniciar Rocky Linux:

```bash
sudo systemctl status mosquitto
```

Debe mostrar:

```text
active (running)
```

Después:

```bash
sudo ss -lntp | grep 1883
```

Después:

```bash
mosquitto_sub \
-h localhost \
-p 1883 \
-u <MQTT_USER> \
-P '<MQTT_PASSWORD>' \
-t "maim/#" \
-v
```

Encender ESP32.

Deben comenzar a aparecer mensajes.

---

# 62. Secuencia mínima de diagnóstico

Si MQTT deja de funcionar:

```text
1. ¿Rocky tiene red?
2. ¿IP del servidor correcta?
3. ¿Mosquitto está running?
4. ¿1883 está escuchando?
5. ¿Firewall permite 1883?
6. ¿usuario/contraseña correctos?
7. ¿topic correcto?
8. ¿ESP32 conectado a Wi-Fi?
9. ¿ESP32 conectado a MQTT?
10. revisar journalctl
```

---

# 63. Estado actual validado

Actualmente se ha comprobado:

```text
[PROBADO] instalación Mosquitto
[PROBADO] servicio systemd
[PROBADO] inicio automático
[PROBADO] listener 1883
[PROBADO] autenticación
[PROBADO] password_file
[PROBADO] conexión local
[PROBADO] Windows → Rocky
[PROBADO] Rocky → Windows
[PROBADO] ESP32 → Mosquitto
[PROBADO] Mosquitto → ESP32
[PROBADO] availability
[PROBADO] Last Will
[PROBADO] telemetry
[PROBADO] state/reported
[PROBADO] event
[PROBADO] command/request
[PROBADO] command/response
[PROBADO] ACK
[PROBADO] DONE
[PROBADO] NACK
[PROBADO] ACK_TIMEOUT
[PROBADO] EXECUTION_TIMEOUT
```

---

# 64. Relación con otros documentos

Servidor Rocky Linux:

```text
02_SERVIDOR_ROCKY_LINUX.md
```

Especificación MQTT:

```text
08_PROTOCOLO_MQTT.md
```

Comandos y transacciones:

```text
12_COMANDOS_Y_TRANSACCIONES.md
```

Timeouts:

```text
13_TIMEOUTS.md
```

Pruebas rápidas:

```text
14_PRUEBAS_Y_DIAGNOSTICO.md
```

---

# 65. Resumen

La configuración actual de laboratorio utiliza:

```text
Rocky Linux 9.8
        │
        ▼
Mosquitto 2.0.22
        │
        ▼
TCP 1883
        │
        ▼
Usuario + contraseña
        │
        ▼
MAIM MQTT Protocol v1
```

Esta infraestructura constituye actualmente el broker MQTT funcional utilizado para desarrollar y validar **MAIM Connectivity v0.1.0**.

La configuración es apropiada para laboratorio y desarrollo, pero deberá reforzarse antes de cualquier despliegue de producción expuesto fuera de una red controlada.
