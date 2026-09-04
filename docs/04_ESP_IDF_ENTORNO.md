# MAIM Connectivity

## Entorno ESP-IDF

**Documento:** 04_ESP_IDF_ENTORNO.md

**Proyecto:** MAIM Connectivity

**Entorno principal:** Windows + Visual Studio Code

**Framework:** ESP-IDF 5.5.5

**Target actual:** ESP32 clásico

**Módulo de referencia:** ESP32-WROOM-32E-N4

**Estado:** [PROBADO] Entorno funcional y estable


---

# 1. Propósito

Este documento describe la instalación, configuración, uso y diagnóstico del entorno de desarrollo utilizado para **MAIM Connectivity**.

El objetivo es poder preparar una computadora nueva y llegar al mismo entorno funcional utilizado actualmente para:

* crear proyectos ESP-IDF;
* compilar firmware;
* configurar el target;
* configurar particiones;
* flashear ESP32;
* utilizar monitor serie;
* diagnosticar errores;
* mantener el proyecto en Visual Studio Code.

---

# 2. Arquitectura del entorno

El entorno actual utiliza:

```text
Windows
   │
   ├── Visual Studio Code
   │      │
   │      └── ESP-IDF Extension
   │
   ├── ESP-IDF 5.5.5
   │
   ├── Python virtual environment
   │
   ├── Git
   │
   ├── CMake
   │
   ├── Ninja
   │
   └── Xtensa ESP32 Toolchain
```

ESP-IDF administra internamente las herramientas necesarias.

No es recomendable configurar manualmente cada ejecutable en el PATH si el entorno oficial ya está disponible.

---

# 3. Versiones utilizadas

La instalación funcional actual utiliza:

```text
ESP-IDF: 5.5.5
```

El entorno mostró:

```text
IDF_PATH:
C:\esp\v5.5.5\esp-idf
```

Herramientas:

```text
IDF_TOOLS_PATH:
C:\Espressif\tools
```

Entorno Python:

```text
IDF_PYTHON_ENV_PATH:
C:\Espressif\tools\python\v5.5.5\venv
```

Durante compilación se utilizaron herramientas como:

```text
CMake 3.30.x
Ninja
esptool.py 4.12.0
xtensa-esp32-elf
```

---

# 4. Visual Studio Code

El editor utilizado es:

```text
Visual Studio Code
```

Se utiliza principalmente para:

* edición de código;
* administración de archivos;
* terminal integrada;
* integración ESP-IDF;
* Git;
* documentación Markdown.

---

# 5. Extensión ESP-IDF

Dentro de VS Code debe instalarse la extensión oficial:

```text
Espressif IDF
```

Después de instalar la extensión, esta debe estar correctamente vinculada con una instalación existente de ESP-IDF.

---

# 6. Problema: extensión instalada pero configuración inválida

Durante la configuración apareció:

```text
The extension configuration is not valid
```

junto con opciones similares a:

```text
Source ESP-IDF
Current ESP-IDF setup is not found
```

Esto significa que la extensión está instalada, pero todavía no conoce la ubicación correcta del framework y herramientas.

---

# 7. Solución: seleccionar instalación ESP-IDF

La instalación funcional se encontraba en:

```text
C:\esp\v5.5.5\esp-idf
```

La extensión debe utilizar esa instalación.

Desde VS Code:

```text
Ctrl + Shift + P
```

buscar comandos relacionados con:

```text
ESP-IDF
```

y seleccionar la instalación existente.

Después debe quedar configurado:

```text
IDF_PATH
Python
Git
CMake
Ninja
Toolchain
```

---

# 8. Crear proyecto

Desde:

```text
Ctrl + Shift + P
```

ejecutar:

```text
ESP-IDF: New Project
```

La ventana presenta normalmente:

```text
ESP-IDF Examples
Templates
```

Para MAIM Connectivity se utilizó:

```text
Templates
```

porque se quería una base limpia.

---

# 9. Nombre y ubicación del proyecto

Proyecto:

```text
maim_connectivity
```

Ubicación utilizada:

```text
C:\MAIM\ESP32\maim_connectivity
```

---

# 10. Estructura inicial

El proyecto creado mostró una estructura similar a:

```text
MAIM_CONNECTIVITY
│
├── .devcontainer
│   ├── devcontainer.json
│   └── Dockerfile
│
├── .vscode
│   ├── c_cpp_properties.json
│   ├── launch.json
│   └── settings.json
│
├── main
│   ├── CMakeLists.txt
│   └── main.c
│
├── .clangd
├── .gitignore
└── CMakeLists.txt
```

Posteriormente se añadieron más módulos.

---

# 11. Target del dispositivo

El módulo utilizado es:

```text
ESP32-WROOM-32E-N4
```

Por lo tanto, el target correcto es:

```text
esp32
```

No utilizar:

```text
esp32s2
esp32s3
esp32c3
```

salvo que el hardware cambie.

---

# 12. Configurar target

Desde la terminal ESP-IDF:

```powershell
idf.py set-target esp32
```

Resultado esperado:

```text
Target set to esp32
```

Este comando puede regenerar configuración y directorios de build.

---

# 13. Problema: `idf.py` no reconocido

Al abrir una terminal PowerShell normal dentro de VS Code apareció:

```text
idf.py : El término 'idf.py' no se reconoce como nombre de un cmdlet...
```

Esto no significa que ESP-IDF esté mal instalado.

Significa que la terminal no tiene cargado el entorno ESP-IDF.

---

# 14. Terminal correcta

Debe utilizarse:

```text
ESP-IDF: Open ESP-IDF Terminal
```

desde:

```text
Ctrl + Shift + P
```

La terminal funcional mostró:

```text
IDF PowerShell Environment
-------------------------

Environment variables set:

IDF_PATH: C:\esp\v5.5.5\esp-idf
IDF_TOOLS_PATH: C:\Espressif\tools
IDF_PYTHON_ENV_PATH: C:\Espressif\tools\python\v5.5.5\venv

Python environment activated.
```

y el prompt:

```text
(venv) PS C:\MAIM\ESP32\maim_connectivity>
```

---

# 15. Activar manualmente el entorno

El entorno también puede cargarse manualmente:

```powershell
& 'C:\Espressif\tools\Microsoft.v5.5.5.PowerShell_profile.ps1'
```

Después debe aparecer:

```text
IDF PowerShell Environment
```

y:

```text
(venv)
```

en el prompt.

---

# 16. Verificar versión

Ejecutar:

```powershell
idf.py --version
```

Resultado esperado:

```text
ESP-IDF v5.5.5
```

---

# 17. Error por sintaxis incorrecta

Durante una prueba se escribió:

```powershell
idf.py -- version
```

con un espacio.

ESP-IDF interpretó:

```text
version
```

como un target Ninja.

Resultado:

```text
ninja: error: unknown target 'version'
```

Correcto:

```powershell
idf.py --version
```

---

# 18. Compilar

Desde la raíz del proyecto:

```powershell
idf.py build
```

ESP-IDF ejecuta:

```text
CMake
Ninja
compilador Xtensa
linker
esptool
```

Resultado esperado:

```text
Project build complete.
```

---

# 19. Primera compilación

La primera compilación normalmente tarda más porque debe:

* configurar CMake;
* preparar componentes;
* generar archivos;
* compilar dependencias.

Las compilaciones posteriores son incrementales y normalmente recompilan solo lo necesario.

---

# 20. `fullclean`

Para eliminar completamente el build generado:

```powershell
idf.py fullclean
```

Después:

```powershell
idf.py build
```

No es necesario utilizar `fullclean` después de cada cambio.

Debe reservarse para:

* cambios importantes de configuración;
* cambios de target;
* problemas de caché;
* cambios de particiones;
* errores extraños de CMake.

---

# 21. Problema: código nuevo no aparece en ESP32

Durante el desarrollo se modificaron archivos, pero después de compilar y flashear el ESP32 seguía ejecutando el código anterior.

El log mostraba:

```text
Firmware v0.1
ESP32 funcionando correctamente
```

aunque el código nuevo ya debería mostrar Wi-Fi.

La causa fue:

```text
los archivos modificados no habían sido guardados
```

VS Code estaba compilando la última versión guardada en disco.

---

# 22. Solución: guardar antes de compilar

Antes de cada build:

```text
Ctrl + S
```

o:

```text
File → Save All
```

Recomendación:

```text
Modificar
↓
Guardar
↓
Compilar
↓
Flashear
```

---

# 23. Identificar cambios no guardados

VS Code muestra normalmente un punto o indicador en la pestaña del archivo cuando existen cambios pendientes.

Antes de una compilación importante se recomienda:

```text
Save All
```

---

# 24. Configuración CMake del proyecto

El archivo principal:

```text
CMakeLists.txt
```

de la raíz contiene normalmente:

```cmake
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(maim_connectivity)
```

---

# 25. `main/CMakeLists.txt`

Actualmente el proyecto incluye módulos como:

```cmake
idf_component_register(
    SRCS
        "main.c"
        "wifi_manager.c"
        "mqtt_manager.c"
        "time_manager.c"
        "uart_protocol.c"
        "device_manager.c"
        "transaction_manager.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        mqtt
        json
)
```

Cuando se agrega un nuevo archivo `.c`, debe incorporarse aquí.

---

# 26. Problema: nuevo archivo no se compila

Si se crea:

```text
nuevo_modulo.c
```

pero no se añade a:

```text
main/CMakeLists.txt
```

ESP-IDF no lo incluirá automáticamente.

Debe agregarse dentro de:

```cmake
SRCS
```

Ejemplo:

```cmake
SRCS
    "main.c"
    "nuevo_modulo.c"
```

---

# 27. Verificar que un archivo se compiló

Durante:

```powershell
idf.py build
```

pueden aparecer entradas similares a:

```text
nuevo_modulo.c.obj
```

Esto confirma que el archivo forma parte del build.

---

# 28. Configuración mediante `menuconfig`

Abrir:

```powershell
idf.py menuconfig
```

Permite configurar parámetros de ESP-IDF.

Se utilizó para:

```text
Flash size
Partition Table
SNTP
```

---

# 29. Navegación en menuconfig

Usar:

```text
Flechas
Enter
Space
Esc
```

Para buscar una opción:

```text
/
```

y escribir el nombre.

Ejemplo:

```text
SNTP_MAX_SERVERS
```

---

# 30. Configurar Flash de 4 MB

El ESP32-WROOM-32E-N4 utiliza:

```text
4 MB Flash
```

Inicialmente el build estaba configurado como:

```text
--flash-size 2MB
```

lo cual provocó problemas al validar la tabla de particiones.

---

# 31. Solución Flash Size

Ejecutar:

```powershell
idf.py menuconfig
```

Ir a:

```text
Serial flasher config
→ Flash size
→ 4 MB
```

Guardar.

Después:

```powershell
idf.py fullclean
idf.py build
```

El log correcto debe mostrar:

```text
SPI Flash Size : 4MB
```

---

# 32. Verificar sdkconfig

Puede verificarse con:

```powershell
findstr FLASHSIZE sdkconfig
```

Debe aparecer algo equivalente a:

```text
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="4MB"
```

---

# 33. Tabla de particiones personalizada

Se creó:

```text
partitions.csv
```

en la raíz del proyecto.

ESP-IDF debe configurarse para usar:

```text
Custom partition table CSV
```

desde:

```powershell
idf.py menuconfig
```

---

# 34. Particiones actuales

La tabla validada es:

```text
nvs       24K
otadata    8K
phy_init   4K
ota_0   1536K
ota_1   1536K
avr_fw   832K
```

Más información:

```text
05_PARTICIONES_ESP32.md
```

---

# 35. Error SNTP por número de servidores

Durante la implementación de SNTP apareció:

```text
Tried to configure more servers than enabled in lwip.
Please update CONFIG_SNTP_MAX_SERVERS
```

La inicialización terminaba con:

```text
ESP_ERR_INVALID_ARG
```

---

# 36. Solución SNTP

Abrir:

```powershell
idf.py menuconfig
```

Buscar:

```text
SNTP_MAX_SERVERS
```

y aumentar el número de servidores permitidos.

Después compilar nuevamente.

El problema no era conectividad con el servidor NTP.

Era una limitación de configuración interna de lwIP.

---

# 37. `sdkconfig`

El archivo:

```text
sdkconfig
```

contiene la configuración efectiva del proyecto.

Para este proyecto se decidió versionarlo mediante Git porque contiene parámetros necesarios para reproducir el firmware.

Debe ignorarse únicamente:

```text
sdkconfig.old
```

---

# 38. Flashear ESP32

Comando general:

```powershell
idf.py -p <PORT> flash
```

Ejemplo utilizado:

```powershell
idf.py -p COM6 flash
```

---

# 39. Flash + monitor

```powershell
idf.py -p COM6 flash monitor
```

Este comando:

```text
compila si es necesario
↓
flashea
↓
reinicia
↓
abre monitor serie
```

---

# 40. Identificar puertos COM

Desde PowerShell:

```powershell
[System.IO.Ports.SerialPort]::getportnames()
```

Puede devolver:

```text
COM3
COM6
```

Seleccionar el puerto correspondiente al programador/dispositivo ESP32.

---

# 41. Baud del monitor

El monitor actual utiliza:

```text
115200 baud
```

El log puede mostrar:

```text
esp-idf-monitor ... 115200
```

---

# 42. Log de arranque

Un arranque correcto muestra información como:

```text
ESP-IDF v5.5.5
SPI Speed : 40MHz
SPI Mode  : DIO
SPI Flash Size : 4MB
```

También muestra tabla de particiones.

---

# 43. Información de aplicación

Durante boot aparece:

```text
Project name: maim_connectivity
ESP-IDF: v5.5.5
```

Esto permite comprobar que el firmware correcto está instalado.

---

# 44. Monitor serie

`idf.py monitor` permite:

* leer logs;
* observar eventos;
* introducir caracteres;
* realizar pruebas UART;
* diagnosticar crashes.

---

# 45. Salir del monitor

El monitor muestra normalmente:

```text
Quit: Ctrl+]
Menu: Ctrl+T
```

En teclados español/latino la combinación:

```text
Ctrl + ]
```

puede ser incómoda o provocar otra acción dentro de VS Code.

---

# 46. Método recomendado para salir

Presionar:

```text
Ctrl + T
```

y después:

```text
X
```

sin mantener Ctrl.

Es decir:

```text
Ctrl+T
↓
X
```

Esto cierra el monitor y regresa a PowerShell.

---

# 47. Ayuda del monitor

Presionar:

```text
Ctrl + T
```

y después:

```text
H
```

o según la versión:

```text
Ctrl + H
```

El encabezado del monitor indica la combinación exacta disponible.

---

# 48. Problema con `Ctrl + ]`

Durante las pruebas esta combinación producía zoom en VS Code debido a la distribución del teclado.

Por ello se adoptó:

```text
Ctrl+T → X
```

como método habitual para salir.

---

# 49. Entrada manual mediante monitor serie

Durante el desarrollo del protocolo UART se utilizó el monitor ESP-IDF como simulador de ATmega.

Ejemplo:

```text
<STATE,MODE,STANDBY>
```

El ESP32 recibía los caracteres mediante `stdin` y los procesaba con:

```text
uart_protocol
```

---

# 50. Verificación del parser UART

Ejemplo de salida:

```text
UART_PROTOCOL: Trama valida | Tipo=STATE | Campos=2
Campo[0] = MODE
Campo[1] = STANDBY
```

Esto permitió validar el protocolo sin disponer todavía del controlador físico.

---

# 51. Terminal serie compartida

Actualmente la consola de depuración ESP-IDF utiliza:

```text
GPIO1 → TX0
GPIO3 → RX0
```

El boot muestra:

```text
GPIO 3 and 1 are used as console UART I/O pins
```

Esto es relevante porque UART0 no debe utilizarse simultáneamente como consola y como enlace limpio con un controlador externo sin una estrategia específica.

---

# 52. UART definitivo previsto

Para MAIM Mini se prevé utilizar:

```text
UART2
```

con pines separados.

Esto permitirá mantener UART0 dedicado a:

```text
programación
monitor
debug
```

y UART2 para:

```text
ESP32 ↔ ATmega
```

Estado:

```text
[PLANEADO]
```

---

# 53. Compilación incremental

Después de modificar código normalmente basta:

```powershell
idf.py build
```

ESP-IDF recompila solo los componentes afectados.

No es necesario hacer:

```powershell
idf.py fullclean
```

cada vez.

---

# 54. Tamaño del firmware

ESP-IDF muestra al final del build:

```text
maim_connectivity.bin binary size ...
```

También compara el tamaño contra la partición disponible.

Ejemplo inicial:

```text
Smallest app partition is 0x180000 bytes
```

Esto permite vigilar el crecimiento del firmware.

---

# 55. Importancia del tamaño

Actualmente cada partición OTA tiene:

```text
1.5 MB
```

Por lo tanto, cada nuevo módulo debe considerarse respecto al espacio disponible.

Componentes que incrementan notablemente tamaño:

```text
Wi-Fi
MQTT
JSON
BLE
TLS
OTA
```

---

# 56. Build artifacts

El directorio:

```text
build/
```

contiene archivos generados.

Ejemplos:

```text
bootloader.bin
partition-table.bin
maim_connectivity.bin
maim_connectivity.elf
```

No deben modificarse manualmente.

---

# 57. `.gitignore`

El directorio:

```text
build/
```

debe estar ignorado por Git.

También deben ignorarse:

```text
sdkconfig.old
managed_components/
.vscode/
```

según la política actual del repositorio.

---

# 58. Credenciales

Las credenciales no deben mantenerse directamente dentro del archivo versionable principal.

Actualmente se utiliza:

```text
main/maim_config.h
```

para configuración pública.

Y:

```text
main/maim_secrets.h
```

para secretos locales.

---

# 59. Archivo de secretos

`maim_secrets.h` contiene:

```text
Wi-Fi SSID
Wi-Fi Password
MQTT User
MQTT Password
```

Debe estar incluido en:

```text
.gitignore
```

---

# 60. Plantilla de secretos

Se mantiene:

```text
main/maim_secrets.example.h
```

con valores:

```text
CHANGE_ME
```

para poder reproducir la configuración sin exponer credenciales.

---

# 61. Flujo normal de trabajo

Secuencia recomendada:

```text
Abrir VS Code
      ↓
Abrir proyecto
      ↓
Open ESP-IDF Terminal
      ↓
Editar código
      ↓
Save All
      ↓
idf.py build
      ↓
idf.py -p COMx flash monitor
      ↓
Pruebas
      ↓
Salir Ctrl+T → X
      ↓
Actualizar documentación
      ↓
Git commit
```

---

# 62. Diagnóstico: `idf.py` no existe

Síntoma:

```text
idf.py is not recognized
```

Comprobar:

```text
¿La terminal es ESP-IDF Terminal?
```

Después:

```powershell
echo $env:IDF_PATH
```

Debe mostrar una ruta válida.

---

# 63. Diagnóstico: target incorrecto

Ejecutar:

```powershell
idf.py set-target esp32
```

y recompilar.

---

# 64. Diagnóstico: firmware anterior sigue ejecutándose

Revisar:

```text
¿archivos guardados?
¿se compiló el archivo correcto?
¿se flasheó el COM correcto?
```

Puede añadirse temporalmente:

```c
ESP_LOGI(TAG, "BUILD DE PRUEBA");
```

para confirmar qué firmware está ejecutándose.

---

# 65. Diagnóstico: archivo nuevo no aparece

Revisar:

```text
main/CMakeLists.txt
```

y confirmar que el `.c` esté en:

```cmake
SRCS
```

---

# 66. Diagnóstico: errores de CMake

Si aparecen errores inconsistentes:

```powershell
idf.py fullclean
idf.py build
```

Si persisten, revisar:

```text
CMakeLists.txt
sdkconfig
target
partitions.csv
```

---

# 67. Diagnóstico: Flash incorrecta

Si el build muestra:

```text
--flash-size 2MB
```

pero el hardware tiene 4 MB:

```powershell
idf.py menuconfig
```

y corregir Flash Size.

---

# 68. Diagnóstico: tabla de particiones falla

Revisar:

```text
flash size
offsets
tamaños
CSV seleccionado
```

La tabla actual fue validada correctamente con Flash de 4 MB.

---

# 69. Diagnóstico: puerto ocupado

Si `flash` o `monitor` no pueden abrir COM:

* cerrar otro monitor serie;
* cerrar Arduino Serial Monitor;
* cerrar PuTTY/TeraTerm;
* cerrar otra instancia de `idf.py monitor`;
* desconectar y reconectar dispositivo.

---

# 70. Diagnóstico: GDB warning de COM

Durante monitor apareció:

```text
Warning: GDB cannot open serial ports accessed as COMx
Using \\.\COM6 instead
```

Esto no impidió el funcionamiento del monitor.

ESP-IDF utiliza automáticamente:

```text
\\.\COM6
```

---

# 71. Hard Reset

Después de flashear aparece:

```text
Hard resetting via RTS pin...
```

Esto significa que el programador/USB-UART realizó reset automático.

---

# 72. Información del bootloader

El bootloader actual mostró:

```text
Multicore bootloader
chip revision
SPI Speed
SPI Mode
SPI Flash Size
Partition Table
```

Estos datos son útiles para confirmar que hardware y configuración coinciden.

---

# 73. Uso de NVS

El proyecto inicializa:

```text
nvs_flash_init()
```

antes de Wi-Fi.

Esto es necesario para la infraestructura actual.

La partición NVS está definida en:

```text
partitions.csv
```

---

# 74. `menuconfig` y Git

Después de modificar `menuconfig`, revisar:

```powershell
git diff sdkconfig
```

antes de commit.

Esto permite saber exactamente qué configuración cambió.

---

# 75. VS Code Preview para documentación

Los documentos Markdown pueden visualizarse mediante:

```text
Ctrl + Shift + V
```

Esto permite comprobar cómo se renderizarán aproximadamente en GitHub.

---

# 76. Estructura actual del proyecto

La estructura funcional es aproximadamente:

```text
maim_connectivity/
│
├── docs/
│
├── main/
│   ├── main.c
│   ├── maim_config.h
│   ├── maim_secrets.h
│   ├── maim_secrets.example.h
│   ├── wifi_manager.c
│   ├── wifi_manager.h
│   ├── time_manager.c
│   ├── time_manager.h
│   ├── mqtt_manager.c
│   ├── mqtt_manager.h
│   ├── uart_protocol.c
│   ├── uart_protocol.h
│   ├── device_manager.c
│   ├── device_manager.h
│   ├── transaction_manager.c
│   └── transaction_manager.h
│
├── partitions.csv
├── sdkconfig
├── CMakeLists.txt
├── README.md
└── .gitignore
```

---

# 77. Comandos de consulta rápida

## Activar entorno

```powershell
& 'C:\Espressif\tools\Microsoft.v5.5.5.PowerShell_profile.ps1'
```

## Ver versión

```powershell
idf.py --version
```

## Target

```powershell
idf.py set-target esp32
```

## Configuración

```powershell
idf.py menuconfig
```

## Compilar

```powershell
idf.py build
```

## Limpiar

```powershell
idf.py fullclean
```

## Flashear

```powershell
idf.py -p COM6 flash
```

## Flash + monitor

```powershell
idf.py -p COM6 flash monitor
```

## Puertos COM

```powershell
[System.IO.Ports.SerialPort]::getportnames()
```

## Salir monitor

```text
Ctrl+T
X
```

---

# 78. Criterios para considerar funcional el entorno

El entorno está correctamente configurado cuando:

```text
[x] ESP-IDF Terminal abre
[x] IDF_PATH existe
[x] idf.py --version funciona
[x] target esp32 configurado
[x] idf.py build funciona
[x] particiones compilan
[x] Flash configurada a 4 MB
[x] ESP32 puede flashearse
[x] monitor serie funciona
[x] logs de app_main aparecen
[x] entrada serial funciona
```

---

# 79. Problemas encontrados y solución

| Problema                               | Causa                           | Solución                          |
| -------------------------------------- | ------------------------------- | --------------------------------- |
| Extension configuration invalid        | VS Code no conocía ESP-IDF      | Seleccionar instalación existente |
| `idf.py` no reconocido                 | PowerShell normal               | Abrir ESP-IDF Terminal            |
| `idf.py -- version` falla              | Sintaxis incorrecta             | Usar `idf.py --version`           |
| Código anterior sigue ejecutándose     | Archivos no guardados           | `Save All` antes del build        |
| `metrics undeclared` u otros errores C | Orden/alcance de variables      | Revisar declaración antes de uso  |
| Flash detectada/configurada como 2 MB  | `menuconfig` incorrecto         | Seleccionar 4 MB                  |
| SNTP rechaza configuración             | `SNTP_MAX_SERVERS` insuficiente | Aumentar en `menuconfig`          |
| `Ctrl+]` no sale del monitor           | Distribución de teclado         | Usar `Ctrl+T → X`                 |
| Nuevo `.c` no compila                  | No incluido en CMake            | Agregar a `SRCS`                  |
| COM ocupado                            | Otro programa usa puerto        | Cerrar monitor/aplicación         |
| Build inconsistente                    | Caché                           | `idf.py fullclean`                |

---

# 80. Estado actual

```text
[PROBADO] Visual Studio Code
[PROBADO] ESP-IDF 5.5.5
[PROBADO] ESP-IDF Extension
[PROBADO] PowerShell Environment
[PROBADO] idf.py
[PROBADO] target esp32
[PROBADO] CMake
[PROBADO] Ninja
[PROBADO] build
[PROBADO] flash
[PROBADO] monitor
[PROBADO] Flash 4 MB
[PROBADO] custom partition table
[PROBADO] entrada serial
[PROBADO] documentación Markdown
```

El entorno constituye actualmente la base oficial de desarrollo de **MAIM Connectivity v0.1.0**.
