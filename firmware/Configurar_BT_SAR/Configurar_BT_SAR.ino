/* HC-06 clasico: configuracion automatica a 38400.
   TXD modulo -> D13; D12 -> divisor -> RXD modulo.
   USB 115200. Modulo sin enlace Bluetooth. Ver README.md. */
#include <SoftwareSerial.h>
#include <string.h>

const byte BT_RX = 13, BT_TX = 12;
SoftwareSerial bluetooth(BT_RX, BT_TX);
char respuesta[32];
bool confirmado = false;
bool nombreConfirmado = false;
unsigned long ultimoEstado = 0;

void escuchar(unsigned long baud) {
  bluetooth.end();
  delay(100);
  bluetooth.begin(baud);
  Serial.print(F("Probando baud local: ")); Serial.println(baud);
}

bool enviar(const char* comando, const char* esperado) {
  // Intervalo antes de cada AT; sin terminadores para HC-06 clasico.
  delay(1100);
  while (bluetooth.available()) bluetooth.read();
  Serial.print(F("AT> ")); Serial.println(comando);
  bluetooth.print(comando);
  byte n = 0;
  bool desbordado = false;
  unsigned long inicio = millis();
  while (millis() - inicio < 700) {
    if (!bluetooth.available()) continue;
    char c = bluetooth.read();
    if (c == '\r' || c == '\n' || c == ' ') continue;
    if (c == 0 || n >= sizeof(respuesta) - 1) desbordado = true;
    else respuesta[n++] = c;
  }
  respuesta[n] = 0;
  Serial.print(F("RX< "));
  if (n) Serial.println(respuesta); else Serial.println(F("(sin respuesta)"));
  return !desbordado && strcmp(respuesta, esperado) == 0;
}

bool probar(unsigned long baud) {
  escuchar(baud);
  for (byte i = 0; i < 2; ++i) {
    if (enviar("AT", "OK")) return true;
  }
  return false;
}

void informar() {
  if (confirmado) {
    Serial.println(F("BAUD OK: respuesta AT confirmada a 38400."));
    if (nombreConfirmado) {
      Serial.println(F("EXITO: nombre SAR aceptado y baud 38400. Cargar SAR.ino."));
    } else {
      Serial.println(F("NOMBRE NO CONFIRMADO: baud correcto, pero falta acuse OKsetname."));
    }
  } else {
    Serial.println(F("NO CONFIRMADO: revisar enlace desconectado, cables, alimentacion o variante HC-06."));
    Serial.println(F("Solo se prueban 38400 y 9600. R reintenta; no se declara exito sin OK final."));
  }
}

void configurar() {
  confirmado = false;
  nombreConfirmado = false;
  Serial.println(F("CONFIGURACION AUTOMATICA HC-06 - TXD D13 / RXD D12"));
  delay(1500);
  if (probar(38400)) {
    confirmado = true; // Ya configurado: no reescribir baud.
  } else if (probar(9600)) {
    bool acuse = enviar("AT+BAUD6", "OK38400");
    if (!acuse) Serial.println(F("Sin acuse esperado; comprobando directamente a 38400."));
    // Algunos modulos cambian velocidad antes de que se pueda leer el acuse.
    confirmado = probar(38400);
  }
  if (confirmado) nombreConfirmado = enviar("AT+NAMESAR", "OKsetname");
  informar();
  ultimoEstado = millis();
}

void setup() {
  pinMode(9, OUTPUT); pinMode(10, OUTPUT);
  digitalWrite(9, LOW); digitalWrite(10, LOW);
  for (byte pin = 4; pin <= 7; ++pin) {
    pinMode(pin, OUTPUT); digitalWrite(pin, LOW);
  }
  Serial.begin(115200);
  configurar();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r' || c == 'R') configurar();
  }
  if (millis() - ultimoEstado >= 5000) {
    informar(); ultimoEstado = millis();
  }
}
