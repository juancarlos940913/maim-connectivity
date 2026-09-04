# MAIM Connectivity

## Particiones ESP32

**Documento:** 05_PARTICIONES_ESP32.md

**Proyecto:** MAIM Connectivity

**Dispositivo de referencia:** ESP32-WROOM-32E-N4

**Flash:** 4 MB

**Tabla:** `partitions.csv` personalizada

**Estado:** [PROBADO] Tabla funcional y validada

---

# 1. Propósito

Este documento describe la distribución de memoria Flash utilizada por **MAIM Connectivity** en el ESP32.

La tabla de particiones fue diseñada para permitir:

* almacenamiento NVS;
* soporte OTA A/B;
* datos PHY;
* dos particiones de aplicación;
* almacenamiento dedicado para firmware AVR;
* margen suficiente para crecimiento del firmware.

La intención es que la distribución de Flash soporte tanto el firmware actual como futuras funciones de actualización remota.

---

# 2. Hardware de referencia

El módulo utilizado durante el desarrollo es:

```text
ESP32-WROOM-32E-N4
```

La designación:

```text
N4
```

corresponde a una memoria Flash de:

```text
4 MB
```

La configuración de ESP-IDF debe coincidir con este tamaño físico.

---

# 3. Problema inicial: ESP-IDF configurado como 2 MB

Durante la primera implementación de la tabla personalizada, el build mostraba:

```text
--flash-size 2MB
```

aunque el módulo instalado disponía de:

```text
4 MB
```

Esto produjo errores durante la generación y validación de la tabla de particiones.

La causa no estaba en el hardware.

El proyecto ESP-IDF estaba configurado para una Flash incorrecta.

---

# 4. Solución

Ejecutar:

```powershell
idf.py menuconfig
```

Ir a:

```text
Serial flasher config
    ↓
Flash size
    ↓
4 MB
```

Guardar y salir.

Después:

```powershell
idf.py fullclean
idf.py build
```

El log de arranque debe mostrar:

```text
SPI Flash Size : 4MB
```

---

# 5. Verificación mediante `sdkconfig`

Puede comprobarse con:

```powershell
findstr FLASHSIZE sdkconfig
```

El proyecto debe contener una configuración equivalente a:

```text
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="4MB"
```

---

# 6. Tabla actual

El archivo se encuentra en:

```text
partitions.csv
```

La tabla funcional validada es:

```csv
# Name,     Type, SubType, Offset,   Size,      Flags
nvs,        data, nvs,     0x9000,   0x6000,
otadata,    data, ota,     0xF000,   0x2000,
phy_init,   data, phy,     0x11000,  0x1000,
ota_0,      app,  ota_0,   0x20000,  0x180000,
ota_1,      app,  ota_1,   0x1A0000, 0x180000,
avr_fw,     data, 0x40,    0x320000, 0x0D0000,
```

ESP-IDF la reporta como:

```text
# Name, Type, SubType, Offset, Size

nvs,data,nvs,0x9000,24K
otadata,data,ota,0xf000,8K
phy_init,data,phy,0x11000,4K
ota_0,app,ota_0,0x20000,1536K
ota_1,app,ota_1,0x1a0000,1536K
avr_fw,data,64,0x320000,832K
```

---

# 7. Vista general de memoria

La distribución conceptual es:

```text
FLASH ESP32 — 4 MB
0x000000
│
├── Bootloader
│
├── Partition Table
│
├── NVS
│
├── OTA Data
│
├── PHY
│
├── OTA_0
│     └── Firmware ESP32 A
│
├── OTA_1
│     └── Firmware ESP32 B
│
├── AVR_FW
│     └── Firmware futuro ATmega / AVR
│
└── Final Flash
```

---

# 8. Mapa con offsets

```text
0x000000
   │
   │ Bootloader / datos reservados
   │
0x008000
   │ Partition Table
   │
0x009000
   ├── NVS
   │   Tamaño: 0x6000 = 24 KB
   │
0x00F000
   ├── OTA DATA
   │   Tamaño: 0x2000 = 8 KB
   │
0x011000
   ├── PHY INIT
   │   Tamaño: 0x1000 = 4 KB
   │
0x020000
   ├── OTA_0
   │   Tamaño: 0x180000 = 1536 KB
   │
0x1A0000
   ├── OTA_1
   │   Tamaño: 0x180000 = 1536 KB
   │
0x320000
   ├── AVR_FW
   │   Tamaño: 0x0D0000 = 832 KB
   │
0x3F0000
   │
   └── Fin de espacio utilizado
```

La Flash física termina aproximadamente en:

```text
0x400000
```

para 4 MB.

---

# 9. Partición `nvs`

Definición:

```text
Name:    nvs
Type:    data
Subtype: nvs
Offset:  0x9000
Size:    24 KB
```

Esta partición almacena datos persistentes utilizados por ESP-IDF y por el firmware.

Actualmente se utiliza para la infraestructura de:

```text
Wi-Fi
NVS Flash
```

Posteriormente podrá almacenar:

* configuración del dispositivo;
* provisioning;
* parámetros persistentes;
* identificadores;
* configuración de servidor;
* datos de operación.

---

# 10. Inicialización NVS

El firmware realiza:

```c
nvs_flash_init();
```

Si aparece:

```text
ESP_ERR_NVS_NO_FREE_PAGES
```

o:

```text
ESP_ERR_NVS_NEW_VERSION_FOUND
```

el firmware puede borrar e inicializar nuevamente la partición.

Conceptualmente:

```text
Inicializar NVS
      │
      ├── OK
      │
      └── Error compatible
              ↓
          borrar NVS
              ↓
          reinicializar
```

---

# 11. Partición `otadata`

Definición:

```text
Name:    otadata
Type:    data
Subtype: ota
Offset:  0xF000
Size:    8 KB
```

Su función es mantener información relacionada con qué partición OTA debe iniciar.

Permite gestionar:

```text
OTA_0
OTA_1
```

---

# 12. Funcionamiento conceptual OTA

Con dos slots:

```text
OTA_0
OTA_1
```

el ESP32 puede estar ejecutando una versión mientras descarga la siguiente en la otra partición.

Ejemplo:

```text
Ejecutando:
OTA_0 v1.0

Descargando:
OTA_1 v1.1

Validación correcta
        ↓
Próximo boot → OTA_1
```

Posteriormente:

```text
Ejecutando:
OTA_1 v1.1

Nueva actualización:
OTA_0 v1.2
```

Esto permite alternar entre ambas.

---

# 13. Ausencia de partición factory

La tabla actual no incluye:

```text
factory
```

Durante el boot apareció:

```text
No factory image, trying OTA 0
```

Esto es esperado con la distribución actual.

El sistema carga inicialmente:

```text
ota_0
```

---

# 14. Partición `phy_init`

Definición:

```text
Name:    phy_init
Type:    data
Subtype: phy
Offset:  0x11000
Size:    4 KB
```

Se utiliza para datos relacionados con la inicialización PHY del ESP32.

No debe reutilizarse para datos propios del proyecto.

---

# 15. Partición `ota_0`

Definición:

```text
Offset:
0x20000

Tamaño:
0x180000

Equivalente:
1536 KB
```

Es una de las dos particiones de aplicación.

Actualmente el firmware de desarrollo arranca desde:

```text
ota_0
```

---

# 16. Partición `ota_1`

Definición:

```text
Offset:
0x1A0000

Tamaño:
0x180000

Equivalente:
1536 KB
```

Tiene exactamente el mismo tamaño que `ota_0`.

Esto es importante porque cualquier firmware válido para una partición debe caber también en la otra.

---

# 17. Tamaño máximo de aplicación

El firmware ESP32 debe ser menor que:

```text
0x180000 bytes
```

aproximadamente:

```text
1.5 MB
```

por aplicación.

ESP-IDF valida este límite durante:

```powershell
idf.py build
```

---

# 18. Ejemplo de validación del build

En una compilación inicial se obtuvo:

```text
maim_connectivity.bin binary size 0x279b0 bytes.
Smallest app partition is 0x180000 bytes.
0x158650 bytes (90%) free.
```

Esto indicaba que el firmware inicial era muy pequeño respecto al slot disponible.

---

# 19. Crecimiento con Wi-Fi y MQTT

Después de agregar infraestructura de red el binario creció considerablemente.

Durante el flash se observó una escritura aproximada de:

```text
755552 bytes
```

Este crecimiento es esperado porque se incorporaron componentes como:

* Wi-Fi;
* TCP/IP;
* MQTT;
* JSON;
* SNTP;
* FreeRTOS;
* lógica adicional.

---

# 20. Regla de tamaño

El tamaño de referencia no debe evaluarse únicamente por:

```text
Wrote X bytes
```

durante `esptool`.

La referencia oficial para decidir si el firmware cabe debe ser el resultado generado por ESP-IDF durante:

```powershell
idf.py build
```

y su comparación contra:

```text
0x180000
```

---

# 21. Margen recomendado

Aunque el firmware pueda ocupar teóricamente casi toda la partición, no se recomienda diseñar el proyecto permanentemente al límite.

Debe mantenerse margen para:

* nuevas funciones;
* correcciones;
* librerías;
* TLS;
* provisioning;
* OTA;
* diagnósticos;
* seguridad.

---

# 22. Funciones que pueden aumentar significativamente el firmware

Especialmente:

```text
BLE
TLS
HTTPS
OTA
certificados
Web server
provisioning
drivers adicionales
```

Por lo tanto, el tamaño debe revisarse periódicamente.

---

# 23. Partición `avr_fw`

Definición:

```text
Name:
avr_fw

Type:
data

Subtype:
0x40

Offset:
0x320000

Size:
0x0D0000
```

Equivalente a:

```text
832 KB
```

Esta partición se reservó específicamente para futuras actualizaciones del controlador AVR.

---

# 24. Objetivo de `avr_fw`

La arquitectura prevista permitirá:

```text
Servidor
   ↓
ESP32 recibe firmware AVR
   ↓
almacena imagen
   ↓
avr_fw
   ↓
ESP32 valida
   ↓
programa ATmega mediante ISP
```

Estado:

```text
[PLANEADO]
```

---

# 25. Por qué 832 KB para AVR

Un ATmega328P dispone de aproximadamente:

```text
32 KB Flash
```

Por lo tanto:

```text
832 KB
```

es mucho mayor que una única imagen AVR.

Esto deja espacio para futuras estrategias como:

* firmware actual;
* firmware nuevo;
* copia de recuperación;
* metadata;
* hashes;
* firmas;
* manifiestos;
* múltiples imágenes;
* firmwares para diferentes controladores.

---

# 26. No asumir que toda `avr_fw` será una imagen binaria

La partición puede evolucionar hacia una estructura interna.

Ejemplo conceptual:

```text
AVR_FW
│
├── Manifest
│
├── Firmware image
│
├── SHA-256
│
├── Version
│
├── Hardware compatibility
│
└── Metadata
```

Por ello no debe reducirse únicamente considerando que el ATmega usa 32 KB.

---

# 27. Subtype personalizado `0x40`

La partición:

```text
avr_fw
```

utiliza un subtype personalizado:

```text
0x40
```

Por eso durante boot puede aparecer:

```text
Unknown data
```

Ejemplo:

```text
avr_fw   Unknown data   01 40
```

Esto no representa un error.

Simplemente ESP-IDF no tiene un nombre estándar para ese subtype personalizado.

---

# 28. Configurar tabla personalizada

Ejecutar:

```powershell
idf.py menuconfig
```

Ir a:

```text
Partition Table
```

seleccionar:

```text
Custom partition table CSV
```

y configurar:

```text
partitions.csv
```

---

# 29. Problema inicial con offsets

Inicialmente se consideró definir todos los offsets manualmente.

Finalmente se verificó la distribución y la tabla terminó correctamente alineada.

Las particiones de aplicación deben respetar los alineamientos requeridos por ESP-IDF.

---

# 30. Alineamiento

Las particiones de aplicación:

```text
ota_0
ota_1
```

se encuentran en offsets compatibles con la alineación requerida.

Ejemplo:

```text
ota_0 → 0x20000
ota_1 → 0x1A0000
```

No deben moverse arbitrariamente sin validar nuevamente el build.

---

# 31. Bootloader

Durante compilación se obtuvo:

```text
Bootloader binary size 0x6610 bytes.
0x9f0 bytes free.
```

Esto mostró que el bootloader todavía disponía de margen.

El bootloader ocupa un espacio separado antes de la tabla de particiones.

---

# 32. Partition Table

La tabla de particiones se flashea en:

```text
0x8000
```

Durante el proceso de flash aparece:

```text
Writing at 0x00008000
```

---

# 33. OTA initial data

La partición `otadata` se encuentra en:

```text
0xF000
```

Durante flash se puede observar:

```text
Writing at 0x0000f000
```

---

# 34. Aplicación inicial

El firmware se graba actualmente a:

```text
0x20000
```

correspondiente a:

```text
ota_0
```

Durante flash:

```text
Writing at 0x00020000
```

---

# 35. Comando generado por ESP-IDF

Después de compilar, ESP-IDF mostró una secuencia similar a:

```powershell
python -m esptool --chip esp32 ... write_flash `
0x1000 build\bootloader\bootloader.bin `
0x8000 build\partition_table\partition-table.bin `
0xf000 build\ota_data_initial.bin `
0x20000 build\maim_connectivity.bin
```

Normalmente no es necesario ejecutar manualmente este comando.

Puede utilizarse:

```powershell
idf.py -p COM6 flash
```

---

# 36. Ver tabla durante boot

El ESP32 imprime:

```text
Partition Table:
```

seguido de:

```text
nvs
otadata
phy_init
ota_0
ota_1
avr_fw
```

Esto permite comprobar que la tabla grabada corresponde a la esperada.

---

# 37. Ejemplo validado

```text
0 nvs       WiFi data     01 02 00009000 00006000
1 otadata   OTA data      01 00 0000f000 00002000
2 phy_init  RF data       01 01 00011000 00001000
3 ota_0     OTA app       00 10 00020000 00180000
4 ota_1     OTA app       00 11 001a0000 00180000
5 avr_fw    Unknown data  01 40 00320000 000d0000
```

---

# 38. Comprobar build completo

Después de cualquier cambio importante:

```powershell
idf.py build
```

Debe terminar con:

```text
Project build complete.
```

y no mostrar errores de overlap ni tamaño.

---

# 39. Problema: particiones exceden Flash

Si la suma de particiones supera los 4 MB, ESP-IDF debe rechazar la tabla.

No reducir validaciones para forzar el build.

Debe corregirse:

```text
offset
size
número de particiones
```

---

# 40. Problema: aplicación demasiado grande

Si aparece un error indicando que:

```text
application does not fit
```

debe revisarse:

1. tamaño del firmware;
2. tamaño de `ota_0`;
3. tamaño de `ota_1`;
4. funciones incluidas;
5. librerías;
6. optimización.

No aumentar una sola OTA sin considerar la segunda.

---

# 41. Regla OTA

Para conservar OTA A/B:

```text
ota_0 size == ota_1 size
```

La nueva aplicación debe caber en ambas.

---

# 42. No modificar offsets sin motivo

La tabla actual está validada.

Por lo tanto, mientras no exista una necesidad real:

```text
NO cambiar offsets
NO cambiar tamaños
NO mover NVS
NO mover otadata
```

Cualquier cambio debe tratarse como modificación de arquitectura de almacenamiento.

---

# 43. Cambios que sí pueden justificar una nueva tabla

Ejemplos:

```text
firmware ESP32 supera 1.5 MB
se requiere filesystem
se requiere almacenamiento de logs
se requiere mayor NVS
avr_fw cambia de estrategia
se agrega partición de certificados
se agrega recovery image
```

---

# 44. Posible filesystem futuro

Actualmente no existe:

```text
SPIFFS
LittleFS
FAT
```

como partición propia.

Si se necesita almacenamiento de archivos, deberá evaluarse el espacio disponible y posiblemente rediseñar la tabla.

Estado:

```text
[PLANEADO / SEGÚN NECESIDAD]
```

---

# 45. NVS vs filesystem

NVS debe utilizarse para:

```text
claves
valores
configuración
pequeños datos persistentes
```

Un filesystem sería más adecuado para:

```text
archivos
logs grandes
certificados múltiples
recursos
```

No deben confundirse ambas funciones.

---

# 46. OTA ESP32 prevista

La existencia de:

```text
ota_0
ota_1
otadata
```

no significa que OTA ya esté implementado.

Actualmente:

```text
[IMPLEMENTADO] particionado compatible
[PLANEADO] descarga OTA
[PLANEADO] validación OTA
[PLANEADO] rollback
[PLANEADO] firmware/desired
[PLANEADO] firmware/reported
```

---

# 47. Actualización AVR prevista

Igualmente:

```text
[IMPLEMENTADO] partición avr_fw
[PLANEADO] descarga firmware AVR
[PLANEADO] validación
[PLANEADO] ISP desde ESP32
[PLANEADO] rollback AVR
```

---

# 48. Seguridad futura del firmware

Antes de implementar OTA deben considerarse:

* hash;
* validación de versión;
* compatibilidad de hardware;
* integridad;
* autenticidad;
* firma digital;
* rollback;
* protección ante pérdida de energía.

La existencia de espacio Flash no sustituye estas medidas.

---

# 49. Relación con `maim_config`

La tabla de particiones no debe depender de credenciales ni configuración de red.

Debe mantenerse como una decisión de arquitectura de almacenamiento.

---

# 50. Relación con Git

Debe versionarse:

```text
partitions.csv
sdkconfig
```

Esto permite reproducir exactamente la distribución de Flash utilizada por una versión del firmware.

---

# 51. `sdkconfig.old`

Debe permanecer ignorado:

```text
sdkconfig.old
```

porque corresponde a configuraciones anteriores generadas automáticamente.

---

# 52. Cambios de particiones y Git

Antes de realizar un commit después de `menuconfig`:

```powershell
git diff sdkconfig
```

y:

```powershell
git diff partitions.csv
```

Esto permite confirmar qué cambió.

---

# 53. Riesgo de cambiar tabla con dispositivos en campo

Una vez existan dispositivos desplegados, cambiar la tabla de particiones puede convertirse en una operación delicada.

Una actualización normal OTA escribe aplicaciones, pero cambiar la tabla puede modificar:

```text
offsets
datos persistentes
ubicación de imágenes
```

Por lo tanto, futuras migraciones de tabla deben diseñarse explícitamente.

---

# 54. Regla para producción

Una vez congelada una generación de hardware, su esquema de particiones debe tratarse como parte de:

```text
HW/FW compatibility
```

Ejemplo:

```text
MAIM MINI HW 1.x
Partition Layout v1
```

Una nueva distribución podría requerir:

```text
Partition Layout v2
```

---

# 55. Posible versionado futuro

Puede añadirse una identificación documental como:

```text
MAIM_PARTITION_LAYOUT_VERSION = 1
```

Estado:

```text
[PLANEADO]
```

Esto puede facilitar actualizaciones remotas y compatibilidad.

---

# 56. Monitorización del tamaño

Después de cada incorporación importante debe revisarse:

```powershell
idf.py build
```

y guardar como referencia:

```text
Application binary size
Smallest app partition
Free space
```

Especialmente después de agregar:

```text
BLE
TLS
OTA
ISP
```

---

# 57. Umbral de revisión recomendado

Si el firmware comienza a aproximarse de forma sostenida a:

```text
~80 % del slot OTA
```

debe realizarse una revisión arquitectónica del uso de memoria antes de continuar agregando funciones.

Esto no es una limitación impuesta por ESP-IDF, sino un criterio interno prudente para mantener margen de evolución.

---

# 58. RAM y Flash son diferentes

El tamaño de:

```text
maim_connectivity.bin
```

corresponde principalmente al uso de Flash.

No indica directamente cuánta RAM utiliza el sistema en ejecución.

Durante boot ESP-IDF reporta también memoria dinámica disponible.

Ambos recursos deben vigilarse por separado.

---

# 59. Tabla resumida

| Partición  | Uso                       |     Offset |  Tamaño |
| ---------- | ------------------------- | ---------: | ------: |
| `nvs`      | Configuración persistente |   `0x9000` |   24 KB |
| `otadata`  | Selección OTA             |   `0xF000` |    8 KB |
| `phy_init` | Datos PHY                 |  `0x11000` |    4 KB |
| `ota_0`    | Firmware ESP32 A          |  `0x20000` | 1536 KB |
| `ota_1`    | Firmware ESP32 B          | `0x1A0000` | 1536 KB |
| `avr_fw`   | Firmware futuro AVR       | `0x320000` |  832 KB |

---

# 60. Comandos de consulta rápida

## Abrir configuración

```powershell
idf.py menuconfig
```

## Limpiar

```powershell
idf.py fullclean
```

## Compilar

```powershell
idf.py build
```

## Ver tamaño Flash configurado

```powershell
findstr FLASHSIZE sdkconfig
```

## Flashear

```powershell
idf.py -p COM6 flash
```

## Monitor

```powershell
idf.py -p COM6 monitor
```

---

# 61. Verificación después de flash

En boot confirmar:

```text
SPI Flash Size : 4MB
```

Después:

```text
Partition Table:
```

y verificar:

```text
nvs
otadata
phy_init
ota_0
ota_1
avr_fw
```

---

# 62. Procedimiento después de modificar particiones

Si se modifica `partitions.csv`:

```text
1. Save All
2. idf.py fullclean
3. idf.py build
4. revisar tabla generada
5. revisar tamaño firmware
6. flashear dispositivo de prueba
7. comprobar boot
8. comprobar NVS
9. comprobar Wi-Fi
10. comprobar MQTT
11. documentar cambio
12. commit
```

---

# 63. Problemas encontrados

| Problema                                 | Causa                               | Solución                         |
| ---------------------------------------- | ----------------------------------- | -------------------------------- |
| Build indicaba 2 MB                      | Flash configurada incorrectamente   | `menuconfig → Flash Size → 4 MB` |
| Error generando tabla                    | Configuración de Flash incompatible | Corregir Flash y recompilar      |
| `avr_fw` aparece como Unknown data       | Subtype personalizado `0x40`        | Comportamiento esperado          |
| `No factory image`                       | Tabla no tiene factory              | Boot desde `ota_0`, esperado     |
| Riesgo de firmware grande                | Slots de 1.5 MB                     | Monitorizar tamaño               |
| Confusión entre bytes escritos y binario | Esptool comprime datos              | Usar reporte de `idf.py build`   |

---

# 64. Estado actual

```text
[PROBADO] Flash física 4 MB
[PROBADO] ESP-IDF configurado 4 MB
[PROBADO] partitions.csv personalizada
[PROBADO] nvs
[PROBADO] otadata
[PROBADO] phy_init
[PROBADO] ota_0
[PROBADO] ota_1
[PROBADO] avr_fw
[PROBADO] boot desde ota_0
[PROBADO] build con slots de 1536 KB

[PLANEADO] OTA ESP32
[PLANEADO] rollback OTA
[PLANEADO] ISP AVR
[PLANEADO] almacenamiento y validación firmware AVR
```

---

# 65. Relación con otros documentos

Entorno ESP-IDF:

```text
04_ESP_IDF_ENTORNO.md
```

Wi-Fi:

```text
06_WIFI.md
```

Actualización OTA futura:

```text
15_OTA_ESP32.md
```

Actualización AVR futura:

```text
16_ACTUALIZACION_AVR.md
```

---

# 66. Resumen

La distribución actual de Flash proporciona:

```text
4 MB TOTAL

├── sistema y configuración
├── OTA_0 = 1.5 MB
├── OTA_1 = 1.5 MB
└── AVR_FW = 832 KB
```

Esta distribución proporciona una base adecuada para:

```text
firmware actual
+
crecimiento
+
OTA ESP32
+
actualización futura del controlador AVR
```

La tabla se encuentra actualmente validada y **no debe modificarse sin una necesidad arquitectónica concreta**.
