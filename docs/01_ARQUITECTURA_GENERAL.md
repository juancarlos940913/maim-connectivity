# MAIM Connectivity
## Arquitectura General


**Documento:** 01_ARQUITECTURA_GENERAL.md

**Proyecto:** MAIM Connectivity

**Versión de arquitectura:** v0.1.0

**Estado:** [PROBADO] Arquitectura base funcional

---

# 1. Propósito

**MAIM Connectivity** es una arquitectura de comunicaciones diseñada para proporcionar conectividad de red estandarizada a diferentes equipos MAIM.

El sistema utiliza un **ESP32 como coprocesador de conectividad**, separado del controlador principal encargado del funcionamiento físico del equipo.

La arquitectura busca que la conectividad pueda reutilizarse entre diferentes modelos sin obligar a rediseñar completamente el firmware de comunicaciones para cada producto.

El principio general es:

```text
Controlador principal
        │
        │ Protocolo interno MAIM
        ▼
      ESP32
        │
        │ MQTT
        ▼
  Infraestructura MAIM
```

El controlador principal puede ser actualmente un:

```text
ATmega328P
ATmega2560
```

o, en futuras plataformas, cualquier otro controlador que implemente el protocolo interno definido por MAIM.

---

# 2. Objetivos arquitectónicos

La arquitectura fue diseñada con los siguientes objetivos.

## 2.1 Separación de responsabilidades

La lógica física del equipo no debe depender directamente de la conectividad de Internet.

El controlador principal administra:

* sensores;
* botones;
* actuadores;
* válvulas;
* bombas;
* temperatura;
* dispensación;
* protecciones;
* secuencias físicas;
* seguridad local.

El ESP32 administra:

* Wi-Fi;
* comunicación MQTT;
* sincronización de hora;
* comunicación con servidor;
* protocolo UART;
* interpretación de datos;
* eventos;
* comandos remotos;
* transacciones;
* timeouts;
* futuras actualizaciones OTA.

Esto permite que un problema de Internet no afecte directamente el funcionamiento básico del equipo.

---

## 2.2 Reutilización

El mismo firmware de conectividad debe poder adaptarse a distintos equipos MAIM.

Ejemplo:

```text
MAIM MINI
MAIM GLASS
MAIM ELITE
futuros equipos
```

La cantidad de sensores, actuadores o funciones de cada equipo puede cambiar sin modificar la arquitectura general.

---

## 2.3 Modularidad

El firmware se divide en módulos independientes.

Actualmente:

```text
wifi_manager
time_manager
mqtt_manager
uart_protocol
device_manager
transaction_manager
```

Cada módulo tiene una responsabilidad específica.

Esto evita concentrar toda la lógica dentro de `main.c`.

---

## 2.4 Escalabilidad

La arquitectura debe permitir:

* agregar sensores;
* agregar outputs;
* agregar eventos;
* agregar comandos;
* agregar métricas;
* agregar nuevos modelos;
* agregar nuevos controladores;
* agregar funciones OTA;
* agregar seguridad;
* agregar backend;
* agregar almacenamiento histórico.

sin cambiar las bases del protocolo.

---

## 2.5 Diagnóstico

Cada capa debe poder probarse de manera independiente.

Por ejemplo:

```text
Wi-Fi
↓
MQTT
↓
UART Protocol
↓
Device Manager
↓
Events
↓
Transactions
↓
Timeouts
```

Esto facilita localizar fallas sin tratar todo el sistema como una sola unidad.

---

# 3. Arquitectura física

La arquitectura de referencia es:

```text
                         INTERNET / RED
                              │
                              │
                     ┌────────▼────────┐
                     │ Infraestructura │
                     │      MAIM       │
                     └────────┬────────┘
                              │
                              │ MQTT
                              │
                     ┌────────▼────────┐
                     │   MQTT Broker   │
                     │    Mosquitto    │
                     └────────┬────────┘
                              │
                              │ TCP/IP
                              │
                          Wi-Fi LAN
                              │
                     ┌────────▼────────┐
                     │      ESP32      │
                     │                 │
                     │ MAIM            │
                     │ Connectivity    │
                     └────────┬────────┘
                              │
                              │ UART
                              │
                     ┌────────▼────────┐
                     │ Controlador     │
                     │ principal       │
                     │                 │
                     │ ATmega / MCU    │
                     └────────┬────────┘
                              │
             ┌────────────────┼────────────────┐
             │                │                │
             ▼                ▼                ▼
         Sensores         Actuadores       Interfaz
```

---

# 4. Responsabilidad del controlador principal

El controlador principal debe mantener el control del equipo incluso cuando:

```text
Wi-Fi no está disponible
MQTT no está disponible
servidor no responde
ESP32 está reiniciando
Internet está caído
```

El comportamiento local crítico no debe depender del servidor.

El controlador debe administrar directamente funciones como:

* apertura y cierre de válvulas;
* lectura de sensores;
* caudal;
* dispensado;
* botones;
* LEDs;
* buzzer;
* límites de seguridad;
* protecciones;
* secuencias de funcionamiento.

El servidor puede solicitar operaciones, pero el controlador conserva la autoridad final para decidir si una operación es segura y válida.

Ejemplo:

```text
Servidor solicita DISPENSE
        │
        ▼
ESP32 envía CMD
        │
        ▼
Controlador evalúa condiciones
        │
        ├── válido → ACK / ejecuta
        │
        └── inválido → NACK
```

---

# 5. Responsabilidad del ESP32

El ESP32 funciona como puente entre el controlador y la infraestructura de red.

Sus responsabilidades principales son:

```text
Conectividad
Sincronización
Traducción de protocolos
Gestión de estado
Eventos
Comandos
Transacciones
Diagnóstico
Actualización futura
```

El ESP32 no debe convertirse en el controlador físico principal del equipo salvo que una futura plataforma sea diseñada específicamente de esa manera.

---

# 6. Capas de software

La arquitectura de firmware actual puede representarse como:

```text
                    main.c
                      │
          ┌───────────┼───────────┐
          │           │           │
          ▼           ▼           ▼
   wifi_manager  time_manager  mqtt_manager
                                  │
                                  │
                       ┌──────────┴──────────┐
                       │                     │
                       ▼                     ▼
                 device_manager      transaction_manager
                       ▲                     ▲
                       │                     │
                       └──────────┬──────────┘
                                  │
                                  ▼
                           uart_protocol
                                  │
                                  ▼
                           UART físico
                         [futuro UART2]
```

Durante las pruebas actuales, el UART físico se simula utilizando el monitor serie.

---

# 7. `main.c`

`main.c` funciona principalmente como orquestador.

Sus responsabilidades deben mantenerse limitadas a:

* inicializar módulos;
* registrar callbacks;
* crear tareas;
* iniciar servicios;
* mantener funciones generales de supervisión.

No debe contener directamente toda la lógica de:

```text
Wi-Fi
MQTT
UART
eventos
transacciones
parsing
```

La lógica específica debe permanecer dentro de cada módulo correspondiente.

---

# 8. `wifi_manager`

`wifi_manager` administra la conectividad Wi-Fi.

Responsabilidades:

* inicialización de `esp_netif`;
* creación de interfaz Station;
* inicialización del driver Wi-Fi;
* conexión;
* reconexión;
* procesamiento de eventos;
* estado de conexión;
* dirección IP;
* RSSI.

El resto del firmware puede consultar:

```text
¿Wi-Fi conectado?
¿Cuál es la IP?
¿Cuál es el RSSI?
```

sin conocer los detalles del driver Wi-Fi.

---

# 9. `time_manager`

`time_manager` administra la hora del sistema.

Utiliza SNTP para obtener una referencia de tiempo válida.

Proporciona:

```text
Unix timestamp
hora local para diagnóstico
estado de sincronización
```

Los mensajes MQTT utilizan:

```text
Unix timestamp UTC
```

Ejemplo:

```json
{
  "ts": 1786497569
}
```

La hora de red no se utiliza para medir timeouts internos.

---

# 10. `mqtt_manager`

`mqtt_manager` administra la comunicación entre el ESP32 y el broker MQTT.

Responsabilidades:

* crear cliente MQTT;
* autenticación;
* conexión;
* reconexión;
* topics;
* Last Will;
* publicaciones;
* suscripciones;
* procesamiento de comandos;
* generación de payloads JSON.

El protocolo MQTT utilizado es:

```text
MAIM MQTT Protocol v1
```

La raíz actual es:

```text
maim/v1/devices/{device_id}/
```

---

# 11. `uart_protocol`

`uart_protocol` implementa:

```text
MAIM Internal UART Protocol v1
```

Su función es exclusivamente interpretar la estructura sintáctica de las tramas.

Ejemplo:

```text
<STATE,MODE,STANDBY>
```

se convierte internamente en:

```text
Tipo   = STATE
Campo0 = MODE
Campo1 = STANDBY
```

`uart_protocol` no determina si:

```text
STANDBY
```

es un modo válido para un modelo específico.

Esa responsabilidad pertenece a capas superiores.

---

# 12. `device_manager`

`device_manager` interpreta semánticamente las tramas procedentes del controlador.

Mantiene en RAM la representación actual del equipo.

Las categorías actuales son:

```text
STATE
METRIC
SENSOR
OUTPUT
ERROR
```

Ejemplo:

```text
<STATE,MODE,DISPENSING>
<METRIC,WATER_TOTAL_ML,16000>
<OUTPUT,VALVE,1>
```

se almacena internamente como:

```text
STATE
MODE = DISPENSING

METRIC
WATER_TOTAL_ML = 16000

OUTPUT
VALVE = 1
```

Posteriormente `mqtt_manager` utiliza esta información para construir:

```text
state/reported
```

---

# 13. Estado reportado

El estado completo del equipo se publica mediante:

```text
maim/v1/devices/{device_id}/state/reported
```

El ESP32 construye el JSON a partir de:

```text
identidad
estado
métricas
sensores
outputs
red
firmware
errores
```

Ejemplo simplificado:

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
    "water_total_ml": 16000
  },

  "outputs": {
    "valve": true
  },

  "errors": []
}
```

---

# 14. Eventos

Los eventos representan situaciones que ocurrieron en un instante específico.

Ejemplo:

```text
<EVENT,DISPENSE_COMPLETED,287,16287>
```

Ruta:

```text
Controlador
   ↓
UART Protocol
   ↓
Device Manager
   ↓
Callback de evento
   ↓
MQTT Manager
   ↓
Mosquitto
```

El ESP32 agrega:

* Event ID;
* timestamp;
* severity;
* estructura JSON.

El evento se publica inmediatamente.

No espera:

```text
GET_STATE
telemetría periódica
```

---

# 15. `transaction_manager`

`transaction_manager` administra las operaciones solicitadas por el servidor que deben ejecutarse en el controlador principal.

El servidor utiliza un ID tipo:

```text
CMD-000200
```

El protocolo UART utiliza un ID numérico:

```text
15
```

El Transaction Manager mantiene la relación:

```text
MQTT command_id = CMD-000200
UART ID         = 15
```

---

# 16. Flujo servidor → controlador

Ejemplo:

```text
Servidor
   │
   │ command/request
   │ DISPENSE
   ▼
Mosquitto
   │
   ▼
ESP32
   │
   ▼
Transaction Manager
   │
   │ genera
   ▼
<CMD,15,DISPENSE>
   │
   ▼
Controlador
```

---

# 17. ACK

Cuando el controlador acepta el comando responde:

```text
<ACK,15>
```

Esto significa:

> El controlador recibió y aceptó la solicitud.

No significa que la operación haya terminado.

El ESP32 publica:

```text
command/response
status = RECEIVED
```

---

# 18. DONE

Cuando la operación termina:

```text
<DONE,15>
```

El ESP32 publica:

```text
command/response
status = SUCCESS
```

La transacción se libera.

---

# 19. NACK

El controlador puede rechazar un comando.

Ejemplo:

```text
<NACK,15,BUSY>
```

El ESP32 publica:

```text
command/response
status = REJECTED
error.code = BUSY
```

El controlador mantiene la autoridad para rechazar operaciones inseguras o inválidas.

---

# 20. Timeouts

Las transacciones incluyen mecanismos de protección.

## ACK Timeout

Valor inicial:

```text
5000 ms
```

Detecta que el controlador no respondió.

Ruta:

```text
CMD
 ↓
sin ACK
 ↓
ACK_TIMEOUT
 ↓
FAILED
```

## Execution Timeout

Valor inicial:

```text
30000 ms
```

Detecta:

```text
CMD
 ↓
ACK
 ↓
sin DONE
 ↓
EXECUTION_TIMEOUT
 ↓
FAILED
```

Los timeouts utilizan:

```c
esp_timer_get_time()
```

y no dependen del reloj SNTP.

---

# 21. Disponibilidad del ESP32

El estado de conexión MQTT utiliza:

```text
availability
```

Cuando el ESP32 se conecta publica:

```json
{
  "status": "online"
}
```

En caso de desconexión inesperada, Mosquitto utiliza Last Will para publicar:

```json
{
  "status": "offline"
}
```

La detección de desconexión queda actualmente bajo el keepalive normal configurado por MQTT.

---

# 22. Telemetría

La telemetría representa datos periódicos que pueden utilizarse para históricos.

Actualmente se publica información básica como:

```json
{
  "network": {
    "rssi_dbm": -54
  }
}
```

La telemetría no representa necesariamente un snapshot completo del equipo.

La diferencia conceptual es:

```text
state/reported
→ fotografía actual del equipo

telemetry
→ muestras periódicas para históricos

event
→ algo ocurrió

command
→ alguien solicita una acción
```

---

# 23. Topics principales

La estructura general es:

```text
maim/
└── v1/
    └── devices/
        └── {device_id}/
            │
            ├── availability
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

La arquitectura contempla añadir posteriormente:

```text
config/desired
config/reported

firmware/desired
firmware/reported
```

sin romper la estructura principal.

---

# 24. Flujo completo controlador → servidor

Ejemplo de estado:

```text
Controlador
   │
   │ <STATE,MODE,DISPENSING>
   ▼
uart_protocol
   │
   ▼
device_manager
   │
   ▼
cache interna
   │
   ▼
mqtt_manager
   │
   ▼
state/reported
   │
   ▼
Mosquitto
   │
   ▼
Servidor
```

Ejemplo de evento:

```text
Controlador
   │
   │ <EVENT,DISPENSE_COMPLETED,...>
   ▼
uart_protocol
   │
   ▼
device_manager
   │
   ▼
event callback
   │
   ▼
mqtt_manager
   │
   ▼
event
   │
   ▼
Mosquitto
```

---

# 25. Flujo completo servidor → controlador

```text
Servidor
   │
   │ command/request
   ▼
Mosquitto
   │
   ▼
mqtt_manager
   │
   ▼
transaction_manager
   │
   │ <CMD,ID,...>
   ▼
Controlador
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

# 26. Independencia del modelo

Uno de los principios principales es que el protocolo no depende directamente de MAIM MINI.

Por ejemplo, un equipo simple puede enviar:

```text
<METRIC,WATER_TOTAL_ML,16000>
<OUTPUT,VALVE,1>
```

mientras otro modelo puede enviar:

```text
<SENSOR,COLD_TANK_C,7.8>
<SENSOR,HOT_TANK_C,81.5>
<SENSOR,TANK_LEVEL_PCT,74>

<OUTPUT,COMPRESSOR,1>
<OUTPUT,HEATER,0>
<OUTPUT,COLD_VALVE,0>
<OUTPUT,HOT_VALVE,0>
```

La arquitectura general permanece idéntica.

---

# 27. Dependencias entre capas

La dirección recomendada de dependencias es:

```text
uart_protocol
      ↓
device_manager
      ↓
mqtt_manager
```

y:

```text
mqtt_manager
      ↓
transaction_manager
      ↓
uart_protocol
```

Cada módulo debe exponer únicamente las funciones necesarias mediante su archivo `.h`.

---

# 28. Gestión de credenciales

Las credenciales locales no forman parte de la arquitectura versionable.

Actualmente se separan mediante:

```text
maim_config.h
maim_secrets.h
maim_secrets.example.h
```

`maim_config.h` contiene configuración pública.

`maim_secrets.h` contiene:

```text
Wi-Fi SSID
Wi-Fi Password
MQTT User
MQTT Password
```

y debe permanecer fuera de Git.

---

# 29. Memoria Flash

La arquitectura utiliza flash de 4 MB con:

```text
NVS
OTA Data
PHY
OTA_0
OTA_1
AVR_FW
```

La existencia de dos slots OTA está prevista para permitir actualización segura del ESP32.

La partición `avr_fw` está reservada para almacenar firmware destinado al controlador AVR en futuras implementaciones.

---

# 30. Funciones previstas

Las siguientes capacidades forman parte de la evolución prevista, pero todavía no deben considerarse implementadas:

```text
[PLANEADO] UART2 físico definitivo
[PLANEADO] provisioning Wi-Fi
[PLANEADO] OTA ESP32
[PLANEADO] programación ISP del ATmega desde ESP32
[PLANEADO] firmware/desired
[PLANEADO] firmware/reported
[PLANEADO] config/desired
[PLANEADO] config/reported
[PLANEADO] MQTT/TLS
[PLANEADO] credenciales por dispositivo
[PLANEADO] ACL por dispositivo
[PLANEADO] backend Node.js
[PLANEADO] almacenamiento histórico
[PLANEADO] dashboard
```

---

# 31. Principios de seguridad

La seguridad del sistema debe respetar al menos estos principios:

1. El servidor solicita; el controlador valida.
2. Los comandos físicos no deben ejecutarse sin validación local.
3. Los comandos MQTT no deben ser retained.
4. Las transacciones deben tener IDs.
5. Las operaciones deben tener timeouts.
6. Los errores deben poder propagarse al servidor.
7. Las credenciales no deben versionarse.
8. La conectividad no debe sustituir protecciones físicas.
9. El equipo debe poder mantener funciones básicas sin Internet.
10. Las actualizaciones futuras deberán validarse antes de instalarse.

---

# 32. Principio de compatibilidad

Los protocolos deben diseñarse para permitir evolución progresiva.

Agregar:

```text
un sensor
una métrica
un evento
un output
un comando
```

no debe obligar a crear una nueva arquitectura.

Una nueva versión mayor del protocolo solo debe utilizarse cuando exista un cambio incompatible.

---

# 33. Estado actual de validación

Actualmente se ha comprobado físicamente:

```text
[PROBADO] arranque ESP32
[PROBADO] Wi-Fi
[PROBADO] reconexión
[PROBADO] IP
[PROBADO] RSSI
[PROBADO] SNTP
[PROBADO] Unix timestamp
[PROBADO] MQTT
[PROBADO] Last Will
[PROBADO] telemetry
[PROBADO] state/reported
[PROBADO] UART parser
[PROBADO] Device Manager
[PROBADO] eventos inmediatos
[PROBADO] MQTT → UART CMD simulado
[PROBADO] ACK
[PROBADO] DONE
[PROBADO] NACK
[PROBADO] ACK Timeout
[PROBADO] Execution Timeout
```

La comunicación con el controlador principal físico todavía se encuentra simulada mediante entrada manual desde el monitor serie.

---

# 34. Criterio de evolución

Antes de añadir una función nueva debe evaluarse:

```text
¿Pertenece al controlador principal?
¿Pertenece al ESP32?
¿Pertenece al servidor?
¿Modifica UART?
¿Modifica MQTT?
¿Modifica payloads?
¿Requiere un nuevo evento?
¿Requiere un nuevo comando?
¿Necesita timeout?
¿Necesita actualización documental?
```

Esto evita que las responsabilidades del sistema se mezclen conforme el proyecto crezca.

---

# 35. Resumen arquitectónico

MAIM Connectivity se basa en una separación clara:

```text
CONTROL FÍSICO
     │
     ▼
Controlador principal
     │
     │ MAIM Internal UART Protocol
     ▼
ESP32
     │
     │ MAIM MQTT Protocol
     ▼
MQTT Broker
     │
     ▼
Backend / Infraestructura MAIM
```

El controlador mantiene el control del equipo.

El ESP32 proporciona conectividad y traducción.

MQTT proporciona transporte de mensajes.

El servidor proporciona supervisión, administración y futuras funciones remotas.

Esta separación constituye la base de **MAIM Connectivity v0.1.0**.
