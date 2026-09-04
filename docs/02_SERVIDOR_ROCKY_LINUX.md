# MAIM Connectivity

## Servidor Rocky Linux

**Documento:** 02_SERVIDOR_ROCKY_LINUX.md

**Proyecto:** MAIM Connectivity

**Servidor de laboratorio:** Rocky Linux 9.8 Minimal

**Hipervisor:** Oracle VirtualBox

**Estado:** [PROBADO] Entorno estable de laboratorio

---

# 1. Propósito

Este documento describe la instalación, configuración y estabilización del servidor de laboratorio utilizado por **MAIM Connectivity**.

El objetivo es que la máquina virtual pueda reconstruirse desde cero y llegar al mismo estado funcional utilizado durante las pruebas actuales.

El servidor de laboratorio se utiliza para alojar servicios permanentes necesarios para el desarrollo de la infraestructura IoT MAIM.

Actualmente el servicio principal instalado sobre este servidor es:

```text
Mosquitto MQTT Broker
```

Posteriormente podrán añadirse otros servicios como:

```text
Node.js
Backend MAIM
Base de datos
API
Servicios OTA
Monitoreo
```

---

# 2. Arquitectura actual del laboratorio

La arquitectura utilizada durante el desarrollo es:

```text
PC Windows
│
├── Visual Studio Code
├── ESP-IDF
├── Mosquitto Client
│
└── Oracle VirtualBox
        │
        ▼
┌──────────────────────────────┐
│ Rocky Linux 9.8 Minimal      │
│                              │
│ MAIM-IoT-Server              │
│                              │
│ Mosquitto MQTT Broker        │
└──────────────┬───────────────┘
               │
               │ Red LAN
               ▼
            ESP32
```

Durante las pruebas se utilizó una dirección IP del servidor dentro de la red local:

```text
192.168.1.85
```

Esta dirección corresponde al entorno de laboratorio y puede cambiar en otra instalación.

---

# 3. Software utilizado

## Host

Sistema operativo:

```text
Windows
```

Hipervisor:

```text
Oracle VirtualBox
```

## Máquina virtual

Distribución:

```text
Rocky Linux 9.8
```

Edición utilizada:

```text
Minimal ISO
```

La instalación Minimal fue elegida porque el servidor no necesita entorno gráfico.

Esto reduce:

* consumo de memoria;
* almacenamiento;
* procesos innecesarios;
* superficie de ataque;
* mantenimiento.

---

# 4. Descarga de VirtualBox

Durante la instalación de Oracle VirtualBox se presentan normalmente dos grupos de descarga:

```text
VirtualBox Platform Packages
VirtualBox Extension Pack
```

Para instalar VirtualBox en Windows se debe descargar primero el paquete correspondiente a:

```text
Windows hosts
```

dentro de:

```text
VirtualBox Platform Packages
```

El:

```text
VirtualBox Extension Pack
```

es adicional y no es necesario para crear y ejecutar la máquina virtual básica.

Puede instalarse posteriormente si se requieren funciones adicionales.

---

# 5. Imagen de Rocky Linux

Se utilizó:

```text
Rocky Linux 9.8 Minimal ISO
```

La edición Minimal proporciona únicamente los componentes necesarios para un servidor básico.

No se requiere Desktop Environment para este proyecto.

---

# 6. Creación de la máquina virtual

Nombre utilizado:

```text
MAIM-IoT-Server
```

Durante la creación de la VM en VirtualBox puede aparecer:

```text
Proceed with Unattended Installation
```

Para tener control directo sobre la instalación del sistema operativo se recomienda desmarcar esta opción.

De esta forma Rocky Linux se instala manualmente mediante su propio instalador.

Esto permite definir correctamente:

* usuario;
* contraseña;
* almacenamiento;
* red;
* zona horaria;
* configuración del sistema.

---

# 7. Instalación de Rocky Linux

Durante el instalador pueden aparecer advertencias sobre:

```text
Root Password
User Creation
Installation Destination
```

Estas opciones deben configurarse antes de poder iniciar la instalación.

## 7.1 Destino de instalación

Seleccionar el disco virtual creado para la VM.

Para el laboratorio puede utilizarse:

```text
Automatic Partitioning
```

No es necesario crear manualmente particiones avanzadas para esta etapa.

---

## 7.2 Usuario

Crear un usuario administrativo normal.

Este usuario será utilizado posteriormente para:

```text
SSH
sudo
administración
configuración
```

Es recomendable que el usuario pertenezca al grupo con permisos administrativos.

---

## 7.3 Contraseña root

Puede configurarse una contraseña root aunque el trabajo habitual se realice mediante un usuario con `sudo`.

Las credenciales reales no deben documentarse dentro del repositorio.

---

# 8. Primer arranque

Después de instalar Rocky Linux se reinicia la máquina virtual.

En una instalación correcta deberá aparecer una terminal aproximadamente como:

```text
Rocky Linux 9.8 (Blue Onyx)

Kernel 5.14.x

maim-iot-lab login:
```

También pueden aparecer mensajes informativos del kernel.

Por ejemplo:

```text
block dm-0: the capability attribute has been deprecated
```

o mensajes asociados al `clocksource`.

Estos mensajes no necesariamente representan una falla del sistema.

Si el prompt:

```text
login:
```

aparece correctamente, el sistema está esperando las credenciales del usuario.

---

# 9. Problema: "Login incorrect"

Durante la instalación del laboratorio apareció inicialmente:

```text
Login incorrect
```

aunque aparentemente la contraseña era correcta.

El problema real fue el uso del:

```text
teclado numérico
```

La máquina virtual no estaba interpretando correctamente algunas teclas del keypad durante el login.

## Solución

Introducir la contraseña utilizando los números de la fila superior del teclado en lugar del keypad numérico.

Después de hacerlo fue posible iniciar sesión correctamente.

## Recomendación

Si una contraseña aparentemente correcta falla:

1. comprobar `Caps Lock`;
2. comprobar distribución de teclado;
3. evitar inicialmente el teclado numérico;
4. escribir la contraseña con las teclas principales;
5. verificar caracteres especiales.

---

# 10. Configuración de red inicial

Durante la instalación se obtuvo una configuración aproximadamente como:

```text
IPv4:             192.168.1.83/24
Gateway:          192.168.1.254
DNS:              192.168.1.254
```

Posteriormente la dirección utilizada por el servidor de laboratorio quedó en:

```text
192.168.1.85
```

El valor exacto depende del DHCP de la red.

---

# 11. Adaptadores de red VirtualBox

Durante las pruebas se utilizaron dos adaptadores:

```text
Adaptador 1 → NAT
Adaptador 2 → Adaptador puente
```

Esta combinación fue finalmente estable y se mantuvo.

---

# 12. Función del adaptador NAT

NAT permite que la máquina virtual tenga acceso a Internet utilizando la conexión de Windows.

Se utiliza principalmente para:

```text
dnf update
descargas
repositorios
instalación de paquetes
NTP
```

Conceptualmente:

```text
Rocky Linux VM
     │
     ▼
VirtualBox NAT
     │
     ▼
Windows Host
     │
     ▼
Internet
```

---

# 13. Función del adaptador puente

El adaptador puente permite que Rocky Linux aparezca como otro dispositivo dentro de la red física.

Ejemplo:

```text
Router
│
├── PC Windows
│
├── ESP32
│
└── Rocky Linux VM
```

Esto es importante porque el ESP32 debe poder conectarse directamente al broker MQTT utilizando:

```text
192.168.1.85:1883
```

sin depender de redirecciones internas de VirtualBox.

---

# 14. Configuración final recomendada

La configuración estable del laboratorio quedó:

```text
Adapter 1:
NAT

Adapter 2:
Bridged Adapter
```

El adaptador puente debe utilizar la interfaz física del host que realmente conecta a la LAN:

```text
Ethernet
o
Wi-Fi
```

según corresponda.

---

# 15. Tipos de adaptadores VirtualBox

Durante la configuración aparecen diferentes emulaciones de tarjeta:

```text
PCnet-PCI
PCnet-FAST III
Intel PRO/1000 MT Desktop
Intel PRO/1000 T Server
Intel PRO/1000 MT Server
Intel 82583V
```

Para una máquina Rocky Linux moderna es recomendable utilizar una interfaz Intel soportada de forma nativa.

Durante la estabilización del laboratorio se comprobó que la velocidad de red no dependía únicamente de seleccionar un modelo diferente de NIC virtual.

---

# 16. Problema: velocidad extremadamente baja

Inicialmente las descargas dentro de la VM alcanzaban aproximadamente:

```text
37–40 KB/s
```

aunque el host Windows disponía de una conexión muy superior.

Las primeras pruebas de velocidad incluso mostraban valores cercanos a:

```text
0
```

Posteriormente algunas descargas alcanzaron:

```text
~1885 KB/s
```

pero todavía se encontraban por debajo de lo esperado.

---

# 17. Diagnóstico del problema de velocidad

Se probaron diferentes configuraciones de red de VirtualBox:

```text
NAT
Adaptador puente
tipos de adaptador Intel
```

Cambiar solamente el tipo de tarjeta virtual no produjo inicialmente una mejora significativa.

La situación se estabilizó después de actualizar correctamente el sistema y mantener configurados ambos adaptadores:

```text
NAT
+
Bridged Adapter
```

Después de ello las actualizaciones llegaron aproximadamente a:

```text
14 MB/s
```

lo que confirmó que la VM podía utilizar correctamente la conexión disponible.

---

# 18. Recomendación ante velocidad baja

Si Rocky Linux presenta velocidades extremadamente bajas:

1. comprobar conectividad básica;
2. comprobar DNS;
3. revisar NAT;
4. revisar adaptador puente;
5. actualizar Rocky Linux;
6. reiniciar la VM;
7. comprobar nuevamente las descargas.

No asumir inmediatamente que la tarjeta virtual seleccionada es el único problema.

---

# 19. Prueba de conectividad IP

Primero comprobar conexión directa a Internet sin depender de DNS.

Ejecutar en Rocky Linux:

```bash
ping -c 4 8.8.8.8
```

Resultado correcto:

```text
4 packets transmitted
4 received
0% packet loss
```

Durante las pruebas iniciales esta prueba funcionó correctamente.

---

# 20. Prueba de DNS

Después:

```bash
ping -c 4 google.com
```

En una etapa inicial:

```text
8.8.8.8
```

funcionaba correctamente, pero:

```text
google.com
```

no respondía.

Esto indicaba que la conectividad IP existía, pero existía un problema relacionado con resolución DNS o conectividad de nombre.

---

# 21. Diagnóstico de DNS

Puede comprobarse la configuración con:

```bash
cat /etc/resolv.conf
```

o:

```bash
nmcli dev show
```

Los DNS utilizados en el laboratorio fueron proporcionados por el router:

```text
192.168.1.254
```

Si existe conectividad con IP pero no con nombres de dominio, revisar:

```text
DNS
NetworkManager
/etc/resolv.conf
Gateway
```

---

# 22. Comprobar interfaces

Para ver las interfaces:

```bash
ip addr
```

o:

```bash
ip a
```

Para rutas:

```bash
ip route
```

Debe existir una ruta por defecto similar a:

```text
default via 192.168.1.254
```

para la interfaz conectada a la LAN.

---

# 23. NetworkManager

Rocky Linux utiliza NetworkManager.

Comandos útiles:

```bash
nmcli device status
```

```bash
nmcli connection show
```

```bash
nmcli dev show
```

Estos comandos permiten identificar:

* nombre de interfaz;
* dirección IP;
* gateway;
* DNS;
* estado de conexión.

---

# 24. Actualización del sistema

Una vez establecida la conectividad:

```bash
sudo dnf update -y
```

La primera actualización puede descargar una cantidad importante de paquetes.

Después:

```bash
sudo reboot
```

Es recomendable reiniciar después de una actualización importante del sistema.

---

# 25. Problema: SSH deja de conectar

Durante las pruebas, después de cambios de red o reinicios, PowerShell dejó temporalmente de poder conectarse al servidor.

Esto puede ocurrir porque:

* la VM cambió de dirección IP;
* la interfaz puente no está activa;
* `sshd` no está activo;
* firewall bloquea el puerto;
* la VM aún no terminó de arrancar;
* el host está intentando utilizar una IP anterior.

---

# 26. Verificar IP actual

Dentro de Rocky Linux:

```bash
ip addr
```

Identificar la dirección de la interfaz puente.

También puede utilizarse:

```bash
hostname -I
```

Ejemplo:

```text
192.168.1.85
```

---

# 27. Verificar SSH

Comprobar el servicio:

```bash
sudo systemctl status sshd
```

Debe mostrar:

```text
Active: active (running)
```

Si no está activo:

```bash
sudo systemctl enable --now sshd
```

---

# 28. Verificar firewall para SSH

Comprobar servicios permitidos:

```bash
sudo firewall-cmd --list-all
```

Normalmente Rocky Linux permite SSH por defecto.

Si fuera necesario:

```bash
sudo firewall-cmd --permanent --add-service=ssh
sudo firewall-cmd --reload
```

---

# 29. Conexión desde PowerShell

Desde Windows:

```powershell
ssh <USUARIO>@<SERVER_IP>
```

Ejemplo de laboratorio:

```powershell
ssh <USUARIO>@192.168.1.85
```

La primera conexión puede solicitar confirmación de fingerprint.

Responder:

```text
yes
```

y posteriormente introducir la contraseña.

---

# 30. Ventaja de trabajar mediante SSH

Una vez funcionando SSH no es necesario utilizar directamente la consola de VirtualBox para tareas normales.

La administración puede hacerse desde Windows:

```text
PowerShell
     │
     │ SSH
     ▼
Rocky Linux
```

Esto permite:

* copiar comandos;
* pegar configuraciones;
* trabajar cómodamente;
* mantener VirtualBox en segundo plano.

---

# 31. Uso del ratón en VirtualBox

Durante la instalación apareció la duda sobre cómo liberar el mouse de la VM.

VirtualBox utiliza una:

```text
Host Key
```

que normalmente es:

```text
Right Ctrl
```

Dependiendo de la configuración.

Con Guest Additions y ciertas configuraciones el mouse puede integrarse automáticamente.

Sin embargo, para un servidor Minimal la mayor parte del trabajo se realiza mediante terminal y posteriormente mediante SSH.

---

# 32. Problema durante reinicio: "terminated"

Durante uno de los reinicios apareció una pantalla negra con:

```text
terminated
```

y cursor parpadeante.

Esto no necesariamente significó una corrupción del sistema.

La VM pudo iniciarse nuevamente y Rocky Linux cargó normalmente.

En caso de que ocurra:

1. esperar algunos segundos;
2. comprobar estado de la VM;
3. apagar completamente si quedó detenida;
4. iniciar nuevamente desde VirtualBox;
5. comprobar el boot de Rocky Linux.

---

# 33. Hora del sistema

La hora correcta es importante para:

* logs;
* MQTT;
* certificados futuros;
* mantenimiento;
* diagnóstico.

Comprobar:

```bash
timedatectl
```

Debe mostrar información similar a:

```text
Local time
Universal time
Time zone
System clock synchronized
NTP service
```

---

# 34. Configurar zona horaria

Para México central puede utilizarse una zona IANA apropiada, por ejemplo:

```bash
sudo timedatectl set-timezone America/Mexico_City
```

Después:

```bash
timedatectl
```

Durante el laboratorio se verificó que la hora local del servidor fuera correcta antes de continuar con MQTT.

---

# 35. Sincronización de hora

Verificar:

```bash
timedatectl status
```

Si la sincronización está activa debería aparecer algo equivalente a:

```text
System clock synchronized: yes
```

La implementación MQTT del ESP32 utiliza sus propios mecanismos SNTP, pero mantener correctamente sincronizado el servidor facilita el diagnóstico.

---

# 36. Hostname

Es recomendable establecer un hostname identificable.

Ejemplo:

```text
maim-iot-lab
```

Comprobar:

```bash
hostname
```

Para modificarlo:

```bash
sudo hostnamectl set-hostname maim-iot-lab
```

---

# 37. Herramientas útiles

Rocky Minimal no instala todas las herramientas normalmente utilizadas en sistemas Desktop.

Durante la configuración apareció, por ejemplo:

```text
sudo: nano: command not found
```

Esto es normal en instalaciones Minimal.

---

# 38. Instalar Nano

Si se desea utilizar Nano:

```bash
sudo dnf install nano -y
```

Posteriormente:

```bash
sudo nano archivo
```

---

# 39. Alternativas a Nano

Rocky normalmente incluye herramientas como:

```text
vi
vim
```

dependiendo de la instalación.

También pueden editarse archivos utilizando:

```bash
sudo vi archivo
```

Para usuarios que no estén familiarizados con `vi`, instalar `nano` simplifica considerablemente la administración.

---

# 40. Comandos básicos de servicio

El sistema utiliza `systemd`.

Consultar estado:

```bash
sudo systemctl status <SERVICIO>
```

Iniciar:

```bash
sudo systemctl start <SERVICIO>
```

Detener:

```bash
sudo systemctl stop <SERVICIO>
```

Reiniciar:

```bash
sudo systemctl restart <SERVICIO>
```

Habilitar al arranque:

```bash
sudo systemctl enable <SERVICIO>
```

Habilitar e iniciar:

```bash
sudo systemctl enable --now <SERVICIO>
```

---

# 41. Salir de `systemctl status`

Al ejecutar:

```bash
sudo systemctl status <SERVICIO>
```

la información puede abrirse mediante un pager y aparecer:

```text
lines 1-26/26 (END)
```

En ese estado no se puede escribir un nuevo comando directamente.

Para salir presionar:

```text
q
```

Esto devuelve al shell.

---

# 42. Logs del sistema

Para ver logs de un servicio:

```bash
sudo journalctl -u <SERVICIO>
```

Para seguirlos en tiempo real:

```bash
sudo journalctl -u <SERVICIO> -f
```

Ejemplo posterior con Mosquitto:

```bash
sudo journalctl -u mosquitto -f
```

---

# 43. Firewall de Rocky Linux

Rocky utiliza normalmente:

```text
firewalld
```

Comprobar:

```bash
sudo systemctl status firewalld
```

Ver reglas:

```bash
sudo firewall-cmd --list-all
```

Los servicios que se expongan a la LAN deberán abrirse explícitamente cuando corresponda.

Ejemplo futuro MQTT:

```bash
sudo firewall-cmd --permanent --add-port=1883/tcp
sudo firewall-cmd --reload
```

La configuración específica de Mosquitto se documenta en:

```text
03_MQTT_MOSQUITTO.md
```

---

# 44. SELinux

Rocky Linux utiliza SELinux.

Comprobar estado:

```bash
getenforce
```

Valores habituales:

```text
Enforcing
Permissive
Disabled
```

No se recomienda deshabilitar SELinux sin una causa clara.

Si un servicio presenta problemas de permisos se deben revisar primero:

* propietario;
* grupo;
* permisos;
* configuración del servicio;
* logs;
* contexto SELinux.

---

# 45. IP fija vs DHCP

Durante el desarrollo se utilizó una IP proporcionada por la red local.

Para un laboratorio estable existen dos opciones recomendables:

## Reserva DHCP en router

Asociar la MAC de la VM con una IP específica.

Ventaja:

```text
configuración simple
```

## IP estática en Rocky

Configurar manualmente mediante NetworkManager.

Ventaja:

```text
independencia del DHCP
```

Para producción o un servidor permanente debe evitarse depender de una IP que pueda cambiar inesperadamente.

---

# 46. Identificar la MAC

Puede utilizarse:

```bash
ip link
```

o revisar VirtualBox.

Durante la instalación se observó una dirección MAC similar a:

```text
08:00:27:xx:xx:xx
```

Las direcciones generadas por VirtualBox suelen comenzar con el OUI utilizado por VirtualBox.

---

# 47. Prueba completa de red recomendada

Después de iniciar el servidor ejecutar:

```bash
ip addr
```

Después:

```bash
ip route
```

Luego:

```bash
ping -c 4 192.168.1.254
```

Después:

```bash
ping -c 4 8.8.8.8
```

Y finalmente:

```bash
ping -c 4 google.com
```

Esto prueba progresivamente:

```text
interfaz
↓
gateway
↓
Internet
↓
DNS
```

---

# 48. Prueba desde Windows

Desde PowerShell:

```powershell
ping <SERVER_IP>
```

Después:

```powershell
ssh <USUARIO>@<SERVER_IP>
```

Si ambas funcionan, existe conectividad entre Windows y Rocky Linux mediante la LAN.

---

# 49. Prueba desde otros dispositivos

El ESP32 deberá poder alcanzar:

```text
<SERVER_IP>
```

sobre la misma red.

Cuando Mosquitto esté instalado, el puerto será:

```text
1883/TCP
```

La prueba completa MQTT se documenta en:

```text
03_MQTT_MOSQUITTO.md
```

---

# 50. Estado estable actual

La configuración estable del laboratorio quedó conceptualmente:

```text
Oracle VirtualBox
│
├── Adapter 1: NAT
│
└── Adapter 2: Bridged
        │
        ▼
Rocky Linux 9.8 Minimal
        │
        ├── Internet ✓
        ├── DNS ✓
        ├── LAN ✓
        ├── SSH ✓
        ├── hora correcta ✓
        └── systemd ✓
```

IP utilizada durante la etapa MQTT:

```text
192.168.1.85
```

Esta IP debe considerarse:

```text
LABORATORIO
```

y no una dirección universal del proyecto.

---

# 51. Problemas encontrados durante la instalación

Los principales problemas observados fueron:

| Problema                                  | Causa / diagnóstico                           | Solución                                              |
| ----------------------------------------- | --------------------------------------------- | ----------------------------------------------------- |
| `Login incorrect`                         | Entrada incorrecta mediante teclado numérico  | Utilizar teclas numéricas principales                 |
| Ping a IP funciona pero dominio no        | Problema de DNS                               | Revisar DNS, NetworkManager y gateway                 |
| Descargas de ~40 KB/s                     | Configuración/estado inicial de red y sistema | Mantener NAT + puente, actualizar sistema y reiniciar |
| SSH deja de conectar                      | IP/interfaz/servicio posiblemente cambiado    | Revisar IP, `sshd` y red puente                       |
| `nano: command not found`                 | Rocky Minimal no incluye Nano                 | `sudo dnf install nano -y`                            |
| `systemctl status` no permite escribir    | Pager activo                                  | Presionar `q`                                         |
| Pantalla `terminated` después de reinicio | VM detenida/transición de reinicio            | Reiniciar VM y comprobar boot                         |
| Cambios de IP                             | DHCP                                          | Reserva DHCP o IP estática                            |

---

# 52. Procedimiento rápido de recuperación

Si el servidor deja de responder:

## Paso 1 — comprobar VirtualBox

Confirmar que la VM está:

```text
Running
```

## Paso 2 — entrar localmente

Iniciar sesión desde la consola de VirtualBox.

## Paso 3 — comprobar interfaces

```bash
ip addr
```

## Paso 4 — comprobar gateway

```bash
ip route
```

## Paso 5 — comprobar Internet

```bash
ping -c 4 8.8.8.8
```

## Paso 6 — comprobar DNS

```bash
ping -c 4 google.com
```

## Paso 7 — comprobar SSH

```bash
sudo systemctl status sshd
```

## Paso 8 — comprobar firewall

```bash
sudo firewall-cmd --list-all
```

---

# 53. Comandos de consulta rápida

## Información del sistema

```bash
cat /etc/os-release
```

```bash
uname -r
```

## Hostname

```bash
hostname
```

## Dirección IP

```bash
hostname -I
```

## Interfaces

```bash
ip addr
```

## Rutas

```bash
ip route
```

## NetworkManager

```bash
nmcli device status
```

## DNS

```bash
cat /etc/resolv.conf
```

## Hora

```bash
timedatectl
```

## Disco

```bash
df -h
```

## Memoria

```bash
free -h
```

## CPU

```bash
lscpu
```

## Servicios fallidos

```bash
systemctl --failed
```

---

# 54. Criterios para considerar estable el servidor

Antes de instalar servicios MAIM deben cumplirse como mínimo:

```text
[ ] Rocky Linux inicia sin intervención
[ ] Usuario administrativo funciona
[ ] sudo funciona
[ ] LAN funciona
[ ] Internet funciona
[ ] DNS funciona
[ ] SSH funciona
[ ] Hora correcta
[ ] Actualizaciones instaladas
[ ] Dirección IP conocida
[ ] Firewall conocido
```

Una vez cumplidos estos puntos puede instalarse el broker MQTT.

---

# 55. Relación con otros documentos

La instalación y configuración de Mosquitto se encuentra en:

```text
03_MQTT_MOSQUITTO.md
```

El entorno ESP-IDF utilizado en Windows se documenta en:

```text
04_ESP_IDF_ENTORNO.md
```

Los procedimientos operativos completos se concentran en:

```text
14_PRUEBAS_Y_DIAGNOSTICO.md
```

---

# 56. Estado actual

```text
[PROBADO] Rocky Linux 9.8 Minimal
[PROBADO] VirtualBox
[PROBADO] NAT
[PROBADO] Adaptador puente
[PROBADO] LAN
[PROBADO] Internet
[PROBADO] DNS
[PROBADO] SSH
[PROBADO] actualización mediante DNF
[PROBADO] hora local
[PROBADO] ejecución de servicios permanentes
```

La máquina virtual constituye actualmente el **servidor de laboratorio de MAIM Connectivity** sobre el cual funciona Mosquitto y se realizan las pruebas entre ESP32, Windows y la infraestructura MQTT.
