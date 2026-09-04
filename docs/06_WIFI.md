# MAIM Connectivity

## Wi-Fi

**Documento:** 06_WIFI.md

**Proyecto:** MAIM Connectivity

**Módulo:** `wifi_manager`

**Framework:** ESP-IDF 5.5.5

**Modo actual:** Wi-Fi Station (STA)

**Estado:** [PROBADO] Conexión, IP, RSSI y reconexión básica funcionales

---

# 1. Propósito

Este documento describe la implementación y comportamiento de la conectividad Wi-Fi utilizada por **MAIM Connectivity**.

El módulo responsable es:

```text
wifi_manager.c
wifi_manager.h
```

Su objetivo es encapsular toda la lógica relacionada con:

* inicialización de red;
* conexión Wi-Fi;
* eventos;
* reconexión;
* obtención de dirección IP;
* lectura RSSI;
* estado de conexión.

El resto del firmware no necesita conocer directamente los detalles internos del driver Wi-Fi de ESP-IDF.

---

# 2. Posición dentro de la arquitectura

La capa Wi-Fi se encuentra entre el ESP32 y la infraestructura IP.

```text
Servidor MAIM
     │
     │ MQTT / TCP-IP
     ▼
Red local / Internet
     │
     │ Wi-Fi
     ▼
ESP32
     │
     ▼
wifi_manager
```

Sin conectividad Wi-Fi no pueden funcionar servicios como:

```text
SNTP
MQTT
OTA futuro
backend remoto
```

Sin embargo, la pérdida de Wi-Fi no debe detener el funcionamiento físico controlado por el microcontrolador principal.

---

# 3. Responsabilidades de `wifi_manager`

`wifi_manager` administra:

```text
Inicialización TCP/IP
Inicialización Wi-Fi
Modo Station
SSID
Contraseña
Eventos
Conexión
Reconexión
IP
Gateway
Máscara
RSSI
Estado
```

Las capas superiores solo necesitan funciones del tipo:

```text
wifi_manager_init()
wifi_manager_is_connected()
wifi_manager_get_ip()
wifi_manager_get_rssi()
```

---

# 4. Modo de operación actual

El ESP32 trabaja actualmente como:

```text
WIFI_MODE_STA
```

Es decir:

```text
ESP32
   │
   ▼
Access Point / Router
```

El ESP32 no crea actualmente su propia red Wi-Fi.

---

# 5. Flujo de inicialización

La secuencia utilizada es aproximadamente:

```text
app_main()
   │
   ▼
NVS
   │
   ▼
wifi_manager_init()
   │
   ├── esp_netif_init()
   ├── event loop
   ├── interfaz STA
   ├── driver Wi-Fi
   ├── registrar eventos
   ├── configurar credenciales
   ├── WIFI_MODE_STA
   └── esp_wifi_start()
             │
             ▼
         esp_wifi_connect()
```

---

# 6. Dependencia de NVS

Antes de inicializar Wi-Fi se ejecuta:

```c
nvs_flash_init();
```

ESP-IDF utiliza NVS internamente para parte de la infraestructura Wi-Fi.

La secuencia correcta es:

```text
NVS
 ↓
Wi-Fi
```

No debe invertirse.

---

# 7. Configuración de credenciales

Las credenciales no deben escribirse directamente dentro de `wifi_manager.c`.

Actualmente se utilizan definiciones como:

```c
#define MAIM_WIFI_SSID
#define MAIM_WIFI_PASSWORD
```

proporcionadas desde:

```text
maim_secrets.h
```

---

# 8. Separación de secretos

La configuración se divide en:

```text
maim_config.h
→ configuración versionable

maim_secrets.h
→ secretos locales
```

`maim_secrets.h` está excluido mediante:

```text
.gitignore
```

Esto evita almacenar en Git:

```text
SSID real
contraseña Wi-Fi real
```

---

# 9. Plantilla de configuración

El repositorio mantiene:

```text
maim_secrets.example.h
```

Ejemplo:

```c
#define MAIM_WIFI_SSID        "CHANGE_ME"
#define MAIM_WIFI_PASSWORD    "CHANGE_ME"
```

Al clonar el proyecto debe crearse una copia local llamada:

```text
maim_secrets.h
```

---

# 10. Conexión Wi-Fi

Durante el arranque se observó:

```text
WIFI_MANAGER: Inicializando WiFi
```

Posteriormente:

```text
WIFI_MANAGER: Intentando conectar a: <SSID>
```

El driver cambia internamente de estados:

```text
init
 ↓
auth
 ↓
assoc
 ↓
run
```

---

# 11. Resultado de conexión validado

Ejemplo real de laboratorio:

```text
wifi:connected with <SSID>
wifi:security: WPA2-PSK
```

Posteriormente DHCP asignó:

```text
IP:      192.168.1.66
Gateway: 192.168.1.254
Mask:    255.255.255.0
```

y el módulo reportó:

```text
WIFI_MANAGER: WiFi conectado correctamente
WIFI_MANAGER: IP: 192.168.1.66
WIFI_MANAGER: Gateway: 192.168.1.254
WIFI_MANAGER: Mascara: 255.255.255.0
```

---

# 12. Dirección IP

Actualmente la dirección IPv4 se obtiene mediante DHCP.

Ejemplo de laboratorio:

```text
192.168.1.66
```

Este valor no debe considerarse fijo.

Puede cambiar dependiendo del router.

---

# 13. Gateway

Durante las pruebas:

```text
192.168.1.254
```

El gateway es obtenido mediante la configuración DHCP.

---

# 14. Máscara

La red utilizada durante desarrollo:

```text
255.255.255.0
```

equivalente a:

```text
/24
```

---

# 15. RSSI

`wifi_manager` permite consultar la intensidad de señal.

Ejemplo:

```text
RSSI: -54 dBm
```

Durante las pruebas se observaron valores aproximadamente entre:

```text
-48 dBm
-59 dBm
```

lo cual corresponde a una conexión adecuada para laboratorio.

---

# 16. Interpretación general del RSSI

Como referencia práctica:

```text
-30 dBm   excelente
-50 dBm   muy buena
-60 dBm   buena
-70 dBm   utilizable
-80 dBm   débil
-90 dBm   prácticamente inutilizable
```

Estos valores son únicamente orientativos.

La calidad real también depende de:

* interferencia;
* ruido;
* congestión;
* router;
* antena;
* ubicación física.

---

# 17. Uso del RSSI

Actualmente el RSSI se utiliza para:

```text
logs
telemetría MQTT
state/reported
diagnóstico
```

Ejemplo:

```json
{
  "network": {
    "rssi_dbm": -54
  }
}
```

---

# 18. Estado de conexión

La API expone:

```c
wifi_manager_is_connected()
```

que permite consultar:

```text
true  → Wi-Fi conectado
false → Wi-Fi no disponible
```

Esto evita que otras capas tengan que manejar directamente los eventos ESP-IDF.

---

# 19. Obtención de IP

La API utilizada conceptualmente es:

```c
wifi_manager_get_ip(
    buffer,
    buffer_size
);
```

Resultado:

```text
192.168.1.66
```

---

# 20. Lectura de RSSI

La API utilizada es:

```c
wifi_manager_get_rssi();
```

Resultado ejemplo:

```text
-54
```

El valor representa:

```text
dBm
```

---

# 21. Eventos Wi-Fi

ESP-IDF utiliza eventos para notificar cambios de estado.

Los eventos relevantes son conceptualmente:

```text
WIFI_EVENT_STA_START
WIFI_EVENT_STA_DISCONNECTED
IP_EVENT_STA_GOT_IP
```

---

# 22. `WIFI_EVENT_STA_START`

Cuando el driver Wi-Fi inicia correctamente se solicita conexión.

Conceptualmente:

```text
Wi-Fi iniciado
      ↓
esp_wifi_connect()
```

---

# 23. `IP_EVENT_STA_GOT_IP`

Este evento indica que la interfaz obtuvo dirección IP.

A partir de este momento puede considerarse operativa para:

```text
SNTP
MQTT
TCP/IP
```

No basta con estar asociado al Access Point.

---

# 24. Diferencia entre asociación e IP

Es posible estar conectado al AP pero no tener todavía una IP válida.

Por eso la secuencia es:

```text
Autenticación Wi-Fi
      ↓
Asociación
      ↓
DHCP
      ↓
IP
      ↓
Conectividad disponible
```

El firmware espera la IP antes de iniciar servicios superiores.

---

# 25. Secuencia actual de arranque

La secuencia general es:

```text
NVS
 ↓
Wi-Fi
 ↓
esperar conexión
 ↓
obtener IP
 ↓
SNTP
 ↓
MQTT
```

Esto evita intentar MQTT antes de tener conectividad de red.

---

# 26. Espera de Wi-Fi en `main.c`

Durante la etapa actual se utiliza una espera sencilla:

```c
while (!wifi_manager_is_connected())
{
    vTaskDelay(
        pdMS_TO_TICKS(500)
    );
}
```

Después:

```text
WiFi disponible
```

y se continúa con SNTP.

---

# 27. Consideración futura

Esta espera bloquea únicamente la secuencia de inicialización, no el scheduler completo de FreeRTOS.

Sin embargo, en futuras versiones puede evolucionarse hacia una arquitectura todavía más orientada a eventos.

Estado:

```text
[IMPLEMENTADO] espera actual
[PLANEADO] refinamiento según necesidades de power management
```

---

# 28. Reconexión

El módulo contempla reconexión cuando el ESP32 pierde la conexión Wi-Fi.

Conceptualmente:

```text
Wi-Fi conectado
      │
      ▼
DISCONNECTED
      │
      ▼
reintento
      │
      ▼
esp_wifi_connect()
```

---

# 29. Número máximo de reintentos

En `wifi_manager.h` existe actualmente:

```c
#define WIFI_MANAGER_MAX_RETRIES 10
```

Este valor controla la estrategia inicial de reintentos.

Debe documentarse cualquier cambio futuro.

---

# 30. Qué ocurre si Wi-Fi falla

Una falla Wi-Fi no debe:

```text
abrir válvulas
detener protecciones
anular lógica local
bloquear el controlador principal
```

El efecto debe limitarse principalmente a:

```text
MQTT offline
telemetría no enviada
comandos remotos no disponibles
sincronización remota no disponible
```

---

# 31. Filosofía de degradación

El sistema debe degradarse de esta forma:

```text
Internet disponible
→ funciones completas

Wi-Fi disponible pero Internet no
→ LAN potencialmente disponible

Wi-Fi perdido
→ funciones locales continúan

Servidor perdido
→ funciones locales continúan
```

El equipo físico no debe depender permanentemente del backend.

---

# 32. MQTT depende de Wi-Fi

Cuando Wi-Fi está disponible:

```text
MQTT puede conectar
```

Si Wi-Fi se pierde:

```text
MQTT se desconecta
```

Cuando Wi-Fi regresa, ESP-MQTT puede reconectarse según su gestión interna.

---

# 33. Last Will y pérdida Wi-Fi

Si la pérdida de conectividad provoca una desconexión abrupta MQTT, el broker terminará detectando la ausencia y publicará:

```text
offline
```

mediante Last Will.

El tiempo de detección depende del keepalive MQTT.

---

# 34. Warning FT-PSK

Durante la conexión apareció:

```text
FT-PSK present but FT disabled,
falling back to WPA2-PSK
```

Este mensaje no representó una falla.

El router anunciaba capacidades de Fast Transition, mientras el ESP32 continuó utilizando:

```text
WPA2-PSK
```

La conexión se completó correctamente.

---

# 35. Acción ante warning FT-PSK

No se realizó ninguna modificación porque:

```text
Wi-Fi conectó correctamente
RSSI correcto
IP correcta
MQTT funcional
```

Regla:

> No corregir warnings que no representen una falla real sin comprender primero su impacto.

---

# 36. Power Management Wi-Fi

Durante el log apareció:

```text
wifi:pm start, type: 1
```

Esto indica que el driver habilitó mecanismos de administración de energía.

La optimización profunda de consumo todavía no forma parte de esta etapa.

---

# 37. Power management futuro

MAIM Mini funcionará posteriormente con batería, por lo cual será necesario evaluar:

```text
Wi-Fi power save
Modem Sleep
Light Sleep
Deep Sleep
intervalos de conexión
MQTT
despertar por eventos
```

Estado:

```text
[PLANEADO]
```

No modificar estos parámetros todavía sin medir consumo real del hardware definitivo.

---

# 38. Provisioning actual

Actualmente las credenciales Wi-Fi son configuradas mediante:

```text
maim_secrets.h
```

Esto es adecuado para laboratorio.

No es la solución de producción.

---

# 39. Provisioning futuro

Se prevé implementar un mecanismo que permita configurar la red sin recompilar firmware.

Posibles tecnologías:

```text
BLE provisioning
SoftAP
aplicación móvil
NVS
provisioning manager ESP-IDF
```

Estado:

```text
[PLANEADO]
```

---

# 40. Reutilización del firmware

`wifi_manager` no contiene lógica específica de MAIM MINI.

Por tanto puede reutilizarse en:

```text
MAIM MINI
MAIM GLASS
MAIM ELITE
otros modelos
```

mientras utilicen ESP32 compatible con la implementación.

---

# 41. Relación con `maim_config.h`

`maim_config.h` contiene configuración general del proyecto.

Ejemplo:

```c
#define MAIM_DEVICE_ID
#define MAIM_MODEL
#define MAIM_MQTT_HOST
#define MAIM_MQTT_PORT
```

Las credenciales permanecen separadas.

---

# 42. Relación con `time_manager`

`time_manager` se inicia únicamente después de que Wi-Fi esté disponible.

Ruta:

```text
wifi_manager
      ↓
IP disponible
      ↓
time_manager
      ↓
SNTP
```

---

# 43. Relación con `mqtt_manager`

Después de sincronización:

```text
mqtt_manager_init()
```

crea el cliente y conecta con:

```text
<MQTT_HOST>:<MQTT_PORT>
```

---

# 44. Prueba básica de Wi-Fi

Después de:

```powershell
idf.py -p COM6 flash monitor
```

se debe observar:

```text
WIFI_MANAGER: Inicializando WiFi
WIFI_MANAGER: Intentando conectar a: <SSID>
WIFI_MANAGER: WiFi iniciado
```

y posteriormente:

```text
WIFI_MANAGER: WiFi conectado correctamente
```

---

# 45. Prueba de IP

Resultado esperado:

```text
WIFI_MANAGER: IP: 192.168.1.x
```

No debe aparecer:

```text
0.0.0.0
```

una vez declarada la conexión completa.

---

# 46. Prueba de gateway

Resultado:

```text
WIFI_MANAGER: Gateway: 192.168.1.254
```

El valor puede cambiar según la red.

---

# 47. Prueba de RSSI

Durante funcionamiento:

```text
MAIM: WiFi OK | IP: 192.168.1.66 | RSSI: -54 dBm | MQTT: ONLINE
```

Este log permite comprobar simultáneamente:

```text
Wi-Fi
IP
RSSI
MQTT
```

---

# 48. Log periódico actual

Durante desarrollo `main.c` imprime aproximadamente cada 5 segundos:

```text
WiFi OK | IP: ... | RSSI: ... | MQTT: ...
```

Esto es útil para depuración.

En producción puede reducirse para evitar exceso de logs.

---

# 49. Diagnóstico: Wi-Fi nunca conecta

Revisar primero:

```text
SSID correcto
Password correcto
router encendido
red 2.4 GHz disponible
señal suficiente
```

ESP32 clásico utiliza Wi-Fi:

```text
2.4 GHz
```

No puede asociarse directamente a una red exclusivamente 5 GHz.

---

# 50. Diagnóstico: contraseña incorrecta

Síntomas posibles:

```text
auth failure
reintentos
STA_DISCONNECTED
```

Verificar:

```text
maim_secrets.h
```

y confirmar que los archivos hayan sido guardados antes del build.

---

# 51. Diagnóstico: firmware sigue usando credenciales anteriores

Verificar:

```text
Save All
```

Después:

```powershell
idf.py build
idf.py -p COM6 flash monitor
```

Recordar que VS Code compila la última versión guardada en disco.

---

# 52. Diagnóstico: se conecta pero no obtiene IP

Revisar:

```text
DHCP del router
gateway
configuración de red
eventos IP
```

Puede estar asociado al AP sin tener todavía conectividad TCP/IP.

---

# 53. Diagnóstico: IP válida pero MQTT no conecta

En ese caso Wi-Fi probablemente no es el problema.

Revisar:

```text
broker IP
puerto 1883
Mosquitto
firewall
usuario MQTT
password MQTT
```

Consultar:

```text
03_MQTT_MOSQUITTO.md
```

---

# 54. Diagnóstico: Wi-Fi conecta pero SNTP falla

Si existe IP pero SNTP falla, revisar:

```text
Internet
DNS
CONFIG_SNTP_MAX_SERVERS
servidor NTP
```

Durante el desarrollo el error SNTP observado fue de configuración interna y no de Wi-Fi.

---

# 55. Diagnóstico: RSSI muy bajo

Si el valor se aproxima a:

```text
-80 dBm
```

o inferior:

* acercar el equipo al AP;
* revisar antena;
* revisar montaje;
* evitar blindaje metálico;
* revisar orientación del módulo;
* comprobar interferencia.

En diseño de PCB debe respetarse la zona de antena del módulo ESP32.

---

# 56. Consideración de PCB

La antena del ESP32 no debe quedar rodeada innecesariamente por:

```text
cobre
GND
metal
gabinete conductor
cables
fuentes de ruido
```

El diseño físico puede afectar significativamente la calidad Wi-Fi aunque el firmware sea correcto.

---

# 57. Seguridad actual

Actualmente la seguridad Wi-Fi depende de la red utilizada.

Durante laboratorio se conectó mediante:

```text
WPA2-PSK
```

El firmware no almacena las credenciales en el repositorio Git.

---

# 58. Seguridad futura

Para despliegue real debe considerarse:

* provisioning seguro;
* borrado de credenciales;
* cambio remoto controlado;
* protección NVS;
* credenciales por instalación;
* recuperación de red;
* factory reset.

Estado:

```text
[PLANEADO]
```

---

# 59. Factory reset futuro

Debe existir una forma controlada de borrar:

```text
SSID
password
configuración de servidor
provisioning
```

sin borrar innecesariamente otros datos del equipo.

La estrategia todavía no está implementada.

---

# 60. Reconexión tras reinicio del router

La arquitectura debe permitir:

```text
router apagado
      ↓
Wi-Fi perdido
      ↓
reintentos
      ↓
router vuelve
      ↓
Wi-Fi conecta
      ↓
IP
      ↓
MQTT reconecta
```

Esto deberá probarse nuevamente con el hardware definitivo.

---

# 61. Reconexión tras reinicio ESP32

Al reiniciar:

```text
NVS
 ↓
Wi-Fi
 ↓
IP
 ↓
SNTP
 ↓
MQTT
```

El sistema recupera automáticamente conectividad usando las credenciales configuradas.

---

# 62. Prueba realizada

Durante el desarrollo se reinició completamente el ESP32 y se observó:

```text
Wi-Fi conectado
IP asignada
SNTP sincronizado
MQTT conectado
```

sin intervención manual.

Estado:

```text
[PROBADO]
```

---

# 63. Telemetría de red

Actualmente se publica periódicamente:

```json
{
  "schema": 1,
  "ts": 1786493729,
  "network": {
    "rssi_dbm": -54
  }
}
```

Esto permite registrar calidad de señal a lo largo del tiempo.

---

# 64. Red dentro de `state/reported`

Ejemplo:

```json
{
  "network": {
    "type": "wifi",
    "connected": true,
    "rssi_dbm": -52,
    "ip": "192.168.1.66"
  }
}
```

---

# 65. No utilizar IP como identidad

La dirección:

```text
192.168.1.66
```

puede cambiar.

Por tanto, la identidad del dispositivo debe basarse en:

```text
MAIM_DEVICE_ID
```

y no en la IP.

---

# 66. Device ID

Ejemplo actual:

```text
MM_TEST_001
```

El topic MQTT utiliza:

```text
maim/v1/devices/MM_TEST_001/...
```

aunque la IP cambie.

---

# 67. IP del broker actual

Durante laboratorio:

```text
192.168.1.85
```

Esta IP también es temporal.

Actualmente se encuentra en:

```c
#define MAIM_MQTT_HOST "192.168.1.85"
```

Posteriormente deberá evolucionar hacia configuración provisionable o hostname.

---

# 68. Uso futuro de DNS

En producción será preferible utilizar un hostname del tipo:

```text
mqtt.maim.example
```

en lugar de una IP fija.

Ventajas:

```text
migración de servidor
balanceo
TLS
administración centralizada
```

Estado:

```text
[PLANEADO]
```

---

# 69. Posible problema de DNS

Si el ESP32 puede alcanzar IPs pero no nombres de dominio:

```text
Wi-Fi puede estar correcto
```

y el problema puede estar en:

```text
DNS de la red
gateway
Internet
```

Este mismo tipo de problema apareció durante la preparación de Rocky Linux.

---

# 70. Módulo independiente

Regla arquitectónica:

> `wifi_manager` no debe conocer MQTT.

Debe limitarse a proporcionar conectividad y estado.

La relación correcta es:

```text
wifi_manager
      ↓
mqtt_manager
```

no:

```text
wifi_manager
      ↔
lógica MQTT mezclada
```

---

# 71. Errores de Wi-Fi y Device Manager

Actualmente los errores internos de red no se integran directamente dentro del `device_manager`, porque este representa principalmente el controlador físico.

La información de red se añade directamente por `mqtt_manager`.

Esto mantiene separadas:

```text
estado del equipo físico
```

y:

```text
estado de conectividad ESP32
```

---

# 72. Posible diagnóstico remoto futuro

Pueden añadirse métricas como:

```text
wifi_disconnect_count
wifi_reconnect_count
mqtt_disconnect_count
last_disconnect_reason
ip_changes
```

para diagnóstico remoto.

Estado:

```text
[PLANEADO]
```

---

# 73. Estadísticas futuras

Ejemplo conceptual:

```json
{
  "network": {
    "rssi_dbm": -58,
    "wifi_reconnects": 3,
    "mqtt_reconnects": 1
  }
}
```

No implementado todavía.

---

# 74. Prueba recomendada de pérdida Wi-Fi

Cuando exista hardware definitivo:

```text
1. Conectar ESP32.
2. Confirmar MQTT ONLINE.
3. Apagar router/AP.
4. Confirmar pérdida Wi-Fi.
5. Confirmar que firmware no se bloquea.
6. Encender router.
7. Confirmar reconexión.
8. Confirmar nueva IP si aplica.
9. Confirmar MQTT ONLINE.
10. Confirmar state/reported.
```

---

# 75. Prueba recomendada de señal pobre

Debe evaluarse físicamente:

```text
RSSI
latencia MQTT
reconexiones
consumo
```

en condiciones reales de instalación.

El entorno de escritorio no representa necesariamente las condiciones finales del equipo.

---

# 76. Prueba recomendada con Internet caído

Escenario:

```text
Wi-Fi LAN activa
Internet desconectado
```

Debe comprobarse qué servicios siguen funcionando.

Si el broker está en LAN:

```text
MQTT local puede continuar
```

Si el broker está remoto:

```text
MQTT no estará disponible
```

El controlador físico debe continuar funcionando.

---

# 77. Relación futura con batería

En MAIM Mini la estrategia Wi-Fi estará directamente relacionada con autonomía.

Será necesario medir:

```text
consumo conectado
consumo transmitiendo
consumo power-save
consumo light sleep
consumo deep sleep
tiempo de reconexión
```

No optimizar únicamente basándose en teoría.

---

# 78. Comandos de compilación

Después de modificar `wifi_manager`:

```powershell
idf.py build
```

Después:

```powershell
idf.py -p COM6 flash monitor
```

---

# 79. Logs mínimos esperados

Durante arranque:

```text
WIFI_MANAGER: Inicializando WiFi
WIFI_MANAGER: Intentando conectar a: <SSID>
WIFI_MANAGER: WiFi iniciado
```

Después:

```text
WIFI_MANAGER: WiFi conectado correctamente
WIFI_MANAGER: IP: ...
WIFI_MANAGER: Gateway: ...
WIFI_MANAGER: Mascara: ...
```

---

# 80. Criterios de funcionamiento

Wi-Fi se considera funcional cuando:

```text
[x] driver inicia
[x] modo STA activo
[x] autenticación correcta
[x] asociación correcta
[x] DHCP entrega IP
[x] gateway disponible
[x] RSSI puede consultarse
[x] wifi_manager_is_connected() = true
[x] SNTP puede ejecutarse
[x] MQTT puede conectar
```

---

# 81. Problemas encontrados

| Problema                                          | Diagnóstico                                                | Solución                          |
| ------------------------------------------------- | ---------------------------------------------------------- | --------------------------------- |
| Warning FT-PSK                                    | Router ofrece Fast Transition                              | Ignorado; WPA2 funciona           |
| Wi-Fi no usable hasta IP                          | Asociación no implica TCP/IP                               | Esperar `IP_EVENT_STA_GOT_IP`     |
| Credenciales en código versionable                | Riesgo de seguridad                                        | `maim_secrets.h`                  |
| Código viejo seguía usando configuración anterior | Archivo no guardado                                        | `Save All`                        |
| SNTP fallaba después de Wi-Fi                     | No era problema Wi-Fi                                      | Ajustar `SNTP_MAX_SERVERS`        |
| IP cambia                                         | DHCP                                                       | Usar Device ID; no depender de IP |
| UART0 comparte consola                            | No relacionado directamente con Wi-Fi, pero afecta pruebas | UART2 futuro                      |

---

# 82. Estado actual

```text
[PROBADO] inicialización Wi-Fi
[PROBADO] Station mode
[PROBADO] WPA2-PSK
[PROBADO] conexión AP
[PROBADO] DHCP
[PROBADO] IPv4
[PROBADO] gateway
[PROBADO] máscara
[PROBADO] RSSI
[PROBADO] integración SNTP
[PROBADO] integración MQTT
[PROBADO] reinicio y recuperación de conexión

[EN DESARROLLO] estrategia completa de reconexión para producción

[PLANEADO] provisioning
[PLANEADO] cambio remoto de red
[PLANEADO] power management por batería
[PLANEADO] métricas avanzadas de red
```

---

# 83. Relación con otros documentos

Entorno ESP-IDF:

```text
04_ESP_IDF_ENTORNO.md
```

Sincronización de hora:

```text
07_SINCRONIZACION_HORA.md
```

MQTT:

```text
08_PROTOCOLO_MQTT.md
```

Mosquitto:

```text
03_MQTT_MOSQUITTO.md
```

Pruebas:

```text
14_PRUEBAS_Y_DIAGNOSTICO.md
```

---

# 84. Resumen

La conectividad Wi-Fi actual funciona bajo el siguiente esquema:

```text
Credenciales locales
        ↓
wifi_manager
        ↓
WIFI_MODE_STA
        ↓
Router / Access Point
        ↓
DHCP
        ↓
IPv4
        ↓
SNTP
        ↓
MQTT
```

`wifi_manager` proporciona una capa independiente y reutilizable para todos los modelos MAIM que utilicen ESP32.

La implementación actual está validada para laboratorio y constituye la base sobre la que posteriormente se añadirán **provisioning, optimización energética y recuperación avanzada de red**.
