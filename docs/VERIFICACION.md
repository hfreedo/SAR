# Verificación de SAR

Desde la raíz del repositorio, con Arduino CLI instalado:

```powershell
arduino-cli core install arduino:avr
arduino-cli lib install "Adafruit NeoPixel@1.15.5" "ServoTimer2Plus@1.0.0"
arduino-cli compile --fqbn arduino:avr:uno .\firmware\SAR
arduino-cli compile --fqbn arduino:avr:uno .\firmware\Configurar_BT_SAR
```

Para las pruebas de lógica se necesitan Node.js/npm y Arduino AVR Boards instalados en la ubicación habitual de Windows:

```powershell
.\tests\run.ps1
```

El script instala `avr8js` en una carpeta temporal si falta. Compila las pruebas para ATmega328P y las ejecuta en el emulador. Verifica pinout, formatos SAR, objetivos PWM, sensores, controles de luces/servos, timeout y rampas.

Los periféricos están sustituidos por simulaciones: no comprueba las interrupciones reales de las bibliotecas, alimentación, HC-06 físico ni comandos AT. El configurador se comprueba por compilación; su confirmación real debe observarse en el monitor serie.

## Qué se preservó al preparar el repositorio

- Base: firmware combinado SAR utilizado en el proyecto Jasy, variante Bluetooth D13/D12.
- Rampas de respuesta: +8/−12 cada 10 ms, misma mezcla de motores y parser SAR.
- Bibliotecas: SoftwareSerial del core AVR, ServoTimer2Plus 1.0.0, Adafruit NeoPixel 1.15.5.
- Cambios de presentación: nombre SAR, estructura de carpetas y mensajes del configurador.
- No se migraron variantes antiguas, respaldos ni datos de la cuenta local.
