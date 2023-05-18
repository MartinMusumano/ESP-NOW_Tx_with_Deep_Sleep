# ESP-NOW_Tx_with_Deep_Sleep
Funcionamiento general:
Al iniciar por primera vez (o resetar manualmente con EN), se fija el numero de detecciones en 0.
Luego se ingresa directamente al modo Deep Sleep, con dos fuentes para despertar al micro:

- Timer configurable
- Nivel alto en GPIO4 (D4)

Si se da el primer caso, el micro iniciará el Wifi en modo station, protocolo LR y configurará lo
necesario para utilizar ESP-NOW. En este caso se toma una dirección de broadcast.
Luego enviara un payload que contiene la MAC del dispositivo y el número de detecciones.

Si se da el segundo caso, se incrementará el número de detecciones, almacenado en la RTC Mem.

Luego de cualquiera de las dos situaciones, se ingresa inmediatamente a Deep Sleep.
