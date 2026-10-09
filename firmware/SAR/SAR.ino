// Adaptacion directa de Robot_movil_SAR; ver README de esta carpeta.
/*
  SAR - control proporcional RoboLink y sensores
  -------------------------------------------------------------------
  Este sketch es independiente del firmware principal.

  Arduino UNO + HC-06 + L298N + HC-SR04

  Protocolo RoboLink compacto recomendado (terminar cada paquete con \n):
    x:60,y:180,p:160,g:100,m:0,s:0

  Los ejes RoboLink se configuran de 0 a 200, con centro en 100:
    x: 0=izquierda, 100=centro, 200=derecha
    y: 0=reversa, 100=centro, 200=adelante
    p: PWM maximo permitido, 80..255
    g: sensibilidad de giro, 20..150

  Protocolo generico alternativo:
    J,<direccion>,<acelerador>   ambos ejes, rango -100..100
    X,<direccion>               eje de direccion
    Y,<acelerador>              eje de aceleracion/reversa

  Ejemplos:
    J,0,80       adelante
    J,-40,80     adelante e izquierda
    J,60,-45     reversa hacia la derecha
    J,0,0        centro / detener con rampa

  Compatibilidad con botones anteriores:
    F=adelante, V=reversa, I=izquierda, D=derecha, S=stop inmediato
    M=alternar MANUAL / AUTO_SIMPLE

  Calibracion en tiempo de ejecucion:
    CFG,SENS,80       sensibilidad general 20..100
    CFG,TURN,70       sensibilidad de giro 20..100
    CFG,DEAD,10       zona muerta 0..30
    CFG,MAX,160       PWM maximo 80..255
    CFG,MIN,70        PWM minimo util 0..150
    CFG,EXPO,45       curva exponencial 0..100
    CFG,TRIML,0       compensacion motor izquierdo -30..30
    CFG,TRIMR,0       compensacion motor derecho -30..30
    STATUS            mostrar configuracion y estado
    PING              responde PONG

  Monitor Serie USB: 115200 baud
  Bluetooth HC-06:    38400 baud
*/

#include <SoftwareSerial.h>

// ------------------------- PINOUT -------------------------
const byte BT_RX = 13;      // TXD del HC-06 -> D13
const byte BT_TX = 12;      // D12 -> divisor -> RXD del HC-06

const byte IN1 = 4;
const byte IN2 = 5;
const byte IN3 = 6;
const byte IN4 = 7;
const byte ENA = 9;
const byte ENB = 10;

const byte TRIG_PIN = 2;
const byte ECHO_PIN = 3;
const byte IR_IZQ = 8, IR_DER = 11;

SoftwareSerial bluetooth(BT_RX, BT_TX);
const unsigned long BAUD_BT = 38400;

// ---------------------- CALIBRACION ----------------------
int sensibilidad = 100;       // 20..100 %
int sensibilidadGiro = 100;   // 20..150 %
int zonaMuerta = 10;          // 0..30 %
int pwmMaximo = 160;          // limite inicial solicitado
int pwmMinimo = 70;           // ajustar segun arranque real de los motores
int curvaExpo = 45;           // 0=lineal, 100=cuadratica
int trimIzquierdo = 0;        // -30..30 PWM
int trimDerecho = 0;          // -30..30 PWM

const unsigned long TIMEOUT_CONTROL_MS = 250;
// Respuesta agil: 0->160 en 200 ms nominales, conservando paso por cero al invertir.
const unsigned long INTERVALO_RAMPA_MS = 10;
const int PASO_ACELERACION = 8;
const int PASO_FRENADO = 12;

// Adaptacion SAR: sensores consultados tambien en manual, frenado con r:1.
const unsigned long INTERVALO_US_MANUAL_MS = 200;
const unsigned long INTERVALO_US_AUTO_MS = 80;
const int DISTANCIA_AUTO_CM = 22;

// ------------------------- ESTADO -------------------------
enum ModoControl { MODO_MANUAL, MODO_AUTO_SIMPLE };
enum EstadoAuto { AUTO_AVANZAR, AUTO_RETROCEDER, AUTO_GIRAR };

ModoControl modoControl = MODO_MANUAL;
EstadoAuto estadoAuto = AUTO_AVANZAR;

int ejeDireccion = 0;
int ejeAcelerador = 0;
int objetivoIzquierdo = 0;  // PWM firmado -255..255
int objetivoDerecho = 0;
int salidaIzquierda = 0;
int salidaDerecha = 0;

bool controlRecibido = false;
bool giroAutoIzquierda = true;
bool botonModoAnterior = false;
unsigned long ultimoComando = 0;
unsigned long ultimoPasoRampa = 0;
unsigned long ultimoUltrasonico = 0;
unsigned long ultimoEstadoUSB = 0;
unsigned long inicioEstadoAuto = 0;
int distanciaCm = -1;

// Un preset RoboLink puede incluir varias claves en el mismo paquete.
const byte TAM_BUFFER = 128;
char bufferUSB[TAM_BUFFER];
char bufferBT[TAM_BUFFER];
byte longitudUSB = 0;
byte longitudBT = 0;


bool proteccion = true, irIzq = false, irDer = false, bloqueoSensor = false;
unsigned long bytesRX = 0, lineasRX = 0, controlesRX = 0, erroresRX = 0;
char ultimaTrama[128] = {};

void setup();
void loop();
void procesarPuerto(Stream& puerto, char* buffer, byte& longitud, bool desdeBT);
void procesarLinea(char* linea, bool desdeBT);
bool procesarPaqueteRoboLink(char* linea, bool desdeBT);
int normalizarEjeRoboLink(int valor);
bool igual(const char* a, const char* b);
void responder(const __FlashStringHelper* texto, bool responderBT);
void responderTexto(const char* texto, bool responderBT);
void recibirJoystick(int direccion, int acelerador);
void entrarManual();
void calcularObjetivos();
int aplicarCurva(int entrada);
int porcentajeAPwm(int porcentaje, int trim);
void actualizarRampas(unsigned long ahora);
int aproximarSalida(int actual, int objetivo);
void aplicarMotorIzquierdo(int pwmFirmado);
void aplicarMotorDerecho(int pwmFirmado);
void detenerInmediato();
void alternarModo(bool desdeBT);
void actualizarAutomatico(unsigned long ahora);
void fijarObjetivosPorcentaje(int izquierda, int derecha);
void actualizarUltrasonico(unsigned long ahora);
void configurar(const char* parametro, int valor, bool desdeBT);
void enviarConfiguracion(bool desdeBT);
void enviarAyuda(bool desdeBT);
void enviarEstadoUSB(unsigned long ahora);
bool campoSAR(const char* clave, int valor);
void protegerSensores();

// ------------------------- SETUP --------------------------
void setup() {
  Serial.begin(115200);
  bluetooth.begin(BAUD_BT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(IR_IZQ, INPUT_PULLUP); pinMode(IR_DER, INPUT_PULLUP);
  detenerInmediato();
  randomSeed(analogRead(A0));

  Serial.println(F("SAR listo - MANUAL;BT=38400;USB=115200"));
  Serial.println(F("Use J,x,y o X,x / Y,y. Envie ? para ayuda."));
  bluetooth.println(F("ROBOT MOVIL SAR LISTO"));
}

// -------------------------- LOOP --------------------------
void loop() {
  // Los comandos siempre tienen prioridad sobre sensores y movimiento.
  procesarPuerto(Serial, bufferUSB, longitudUSB, false);
  procesarPuerto(bluetooth, bufferBT, longitudBT, true);

  unsigned long ahora = millis();
  actualizarUltrasonico(ahora);

  if (modoControl == MODO_MANUAL) {
    if (controlRecibido && ahora - ultimoComando > TIMEOUT_CONTROL_MS) {
      controlRecibido = false;
      ejeDireccion = 0;
      ejeAcelerador = 0;
      objetivoIzquierdo = 0;
      objetivoDerecho = 0;
      Serial.println(F("EVT;STOP;TIMEOUT"));
    }
    if (controlRecibido) calcularObjetivos();
  } else {
    actualizarAutomatico(ahora);
  }

  protegerSensores();
  actualizarRampas(ahora);
  enviarEstadoUSB(ahora);
}

// ---------------------- RECEPCION SERIE -------------------
void procesarPuerto(Stream& puerto, char* buffer, byte& longitud, bool desdeBT) {
  while (puerto.available() > 0) {
    char c = puerto.read();
    if (desdeBT) ++bytesRX;

    if (c == '\r') continue;

    if (c == '\n') {
      if (longitud > 0) {
        buffer[longitud] = '\0';
        if (desdeBT) { ++lineasRX; memcpy(ultimaTrama, buffer, longitud + 1); }
        procesarLinea(buffer, desdeBT);
        longitud = 0;
      }
      continue;
    }

    if (longitud < TAM_BUFFER - 1) {
      buffer[longitud++] = c;
    } else {
      longitud = 0;
      ++erroresRX;
      responder(F("ERR;BUFFER"), desdeBT);
    }
  }
}

void procesarLinea(char* linea, bool desdeBT) {
  // RoboLink usa: key:value,key:value,...\n
  if (strchr(linea, ':') != NULL && procesarPaqueteRoboLink(linea, desdeBT)) {
    return;
  }

  // Aceptar coma, punto y coma, dos puntos o espacios como separadores.
  for (byte i = 0; linea[i] != '\0'; i++) {
    if (linea[i] == ',' || linea[i] == ';' || linea[i] == ':') linea[i] = ' ';
  }

  char comando[10] = {0};
  char parametro[10] = {0};
  int valor1 = 0;
  int valor2 = 0;
  int cantidad = sscanf(linea, "%9s %9s %d", comando, parametro, &valor1);

  // Formato J direccion acelerador.
  if ((comando[0] == 'J' || comando[0] == 'j') &&
      sscanf(linea, "%9s %d %d", comando, &valor1, &valor2) == 3) {
    recibirJoystick(valor1, valor2);
    return;
  }

  // Formatos X valor e Y valor.
  if ((comando[0] == 'X' || comando[0] == 'x') && cantidad >= 2) {
    // Con dos campos, sscanf anterior guardo el numero como texto.
    valor1 = atoi(parametro);
    recibirJoystick(valor1, ejeAcelerador);
    return;
  }

  if ((comando[0] == 'Y' || comando[0] == 'y') && cantidad >= 2) {
    valor1 = atoi(parametro);
    recibirJoystick(ejeDireccion, valor1);
    return;
  }

  if (igual(comando, "F")) { recibirJoystick(0, 100); return; }
  if (igual(comando, "V")) { recibirJoystick(0, -100); return; }
  if (igual(comando, "I")) { recibirJoystick(-100, 0); return; }
  if (igual(comando, "D")) { recibirJoystick(100, 0); return; }

  if (igual(comando, "S")) {
    entrarManual();
    detenerInmediato();
    responder(F("OK;STOP"), desdeBT);
    return;
  }

  if (igual(comando, "M")) {
    alternarModo(desdeBT);
    return;
  }

  if (igual(comando, "RX")) {
    Serial.print(F("RX;last=")); Serial.println(ultimaTrama);
    return;
  }

  if (igual(comando, "PING")) {
    responder(F("PONG"), desdeBT);
    return;
  }

  if (igual(comando, "STATUS")) {
    enviarConfiguracion(desdeBT);
    return;
  }

  if (igual(comando, "CFG") &&
      sscanf(linea, "%9s %9s %d", comando, parametro, &valor1) == 3) {
    configurar(parametro, valor1, desdeBT);
    return;
  }

  if (igual(comando, "?")) {
    enviarAyuda(desdeBT);
    return;
  }

  ++erroresRX;
  responder(F("ERR;COMANDO"), desdeBT);
}

bool procesarPaqueteRoboLink(char* linea, bool desdeBT) {
  bool reconocido = false;
  bool actualizarControl = false;
  bool recalcularSalida = false;
  bool detenerSolicitado = false;
  bool modoRecibido = false;
  bool botonModo = false;
  int nuevaDireccion = ejeDireccion;
  int nuevoAcelerador = ejeAcelerador;

  char* par = strtok(linea, ",");
  while (par != NULL) {
    char* separador = strchr(par, ':');
    if (separador != NULL) {
      *separador = '\0';
      char* clave = par;
      char* textoValor = separador + 1;

      while (*clave == ' ') clave++;
      while (*textoValor == ' ') textoValor++;
      int valor = atoi(textoValor);

      if (campoSAR(clave, valor)) {
        reconocido = true;
      } else if (igual(clave, "x") || igual(clave, "steer")) {
        nuevaDireccion = normalizarEjeRoboLink(valor);
        actualizarControl = true;
        reconocido = true;
      } else if (igual(clave, "y") || igual(clave, "throttle")) {
        nuevoAcelerador = normalizarEjeRoboLink(valor);
        actualizarControl = true;
        reconocido = true;
      } else if (igual(clave, "p") || igual(clave, "power")) {
        pwmMaximo = constrain(valor, 80, 255);
        if (pwmMinimo > pwmMaximo) pwmMinimo = pwmMaximo;
        recalcularSalida = true;
        reconocido = true;
      } else if (igual(clave, "g") || igual(clave, "turn")) {
        sensibilidadGiro = constrain(valor, 20, 150);
        recalcularSalida = true;
        reconocido = true;
      } else if (igual(clave, "speed")) {
        // Compatibilidad anterior: sensibilidad porcentual, no PWM maximo.
        sensibilidad = valor > 100
          ? constrain(map(valor, 0, 255, 20, 100), 20, 100)
          : constrain(valor, 20, 100);
        recalcularSalida = true;
        reconocido = true;
      } else if (igual(clave, "s") || igual(clave, "stop") || igual(clave, "brake")) {
        if (valor != 0) detenerSolicitado = true;
        reconocido = true;
      } else if (igual(clave, "m") || igual(clave, "mode")) {
        botonModo = valor != 0;
        modoRecibido = true;
        reconocido = true;
      }
    }
    par = strtok(NULL, ",");
  }

  // Detectar flanco de pulsacion para no cambiar 20 veces por segundo.
  if (modoRecibido) {
    if (botonModo && !botonModoAnterior) alternarModo(desdeBT);
    botonModoAnterior = botonModo;
  }

  if (detenerSolicitado) {
    entrarManual();
    detenerInmediato();
  } else if (actualizarControl) {
    // RoboLink suele incluir los ejes neutros en todos los paquetes. En AUTO
    // se ignoran los ceros; mover cualquiera de los joysticks vuelve a MANUAL.
    if (modoControl == MODO_MANUAL || nuevaDireccion != 0 || nuevoAcelerador != 0) {
      recibirJoystick(nuevaDireccion, nuevoAcelerador);
    }
  } else if (recalcularSalida && controlRecibido) {
    calcularObjetivos();
  }

  return reconocido;
}

int normalizarEjeRoboLink(int valor) {
  // Configuracion actual de la app: 0..200, con reposo en 100.
  // El control interno del robot sigue trabajando en -100..100.
  return constrain(valor, 0, 200) - 100;
}

bool igual(const char* a, const char* b) {
  while (*a && *b) {
    char ca = *a++;
    char cb = *b++;
    if (ca >= 'a' && ca <= 'z') ca -= 32;
    if (cb >= 'a' && cb <= 'z') cb -= 32;
    if (ca != cb) return false;
  }
  return *a == '\0' && *b == '\0';
}

void responder(const __FlashStringHelper* texto, bool responderBT) {
  Serial.println(texto);
  if (responderBT) bluetooth.println(texto);
}

void responderTexto(const char* texto, bool responderBT) {
  Serial.println(texto);
  if (responderBT) bluetooth.println(texto);
}

// ----------------------- JOYSTICK -------------------------
void recibirJoystick(int direccion, int acelerador) {
  ++controlesRX;
  entrarManual();
  ejeDireccion = constrain(direccion, -100, 100);
  ejeAcelerador = constrain(acelerador, -100, 100);
  ultimoComando = millis();
  controlRecibido = true;
  calcularObjetivos();
}

void entrarManual() {
  if (modoControl != MODO_MANUAL) {
    modoControl = MODO_MANUAL;
    estadoAuto = AUTO_AVANZAR;
  }
}

void calcularObjetivos() {
  int acelerador = aplicarCurva(ejeAcelerador);
  int direccion = aplicarCurva(ejeDireccion);

  acelerador = acelerador * sensibilidad / 100;
  // Forzar 32 bits: en el UNO, int es de 16 bits y esta multiplicacion
  // puede superar 32767 antes de llegar a la division.
  direccion = (long)direccion * sensibilidad * sensibilidadGiro / 10000L;

  long izquierda;
  long derecha;

  if (acelerador == 0) {
    // Sin aceleracion: giro proporcional sobre el propio eje.
    izquierda = direccion;
    derecha = -direccion;
  } else {
    // En movimiento: mezcla tipo automovil. La rueda exterior acelera y la
    // interior desacelera, pero nunca invierte su sentido durante la curva.
    izquierda = acelerador + direccion;
    derecha = acelerador - direccion;

    if (acelerador > 0) {
      izquierda = max(0L, izquierda);
      derecha = max(0L, derecha);
    } else {
      izquierda = min(0L, izquierda);
      derecha = min(0L, derecha);
    }
  }

  long mayor = max(abs(izquierda), abs(derecha));

  if (mayor > 100) {
    izquierda = izquierda * 100L / mayor;
    derecha = derecha * 100L / mayor;
  }

  objetivoIzquierdo = porcentajeAPwm((int)izquierda, trimIzquierdo);
  objetivoDerecho = porcentajeAPwm((int)derecha, trimDerecho);
}

int aplicarCurva(int entrada) {
  entrada = constrain(entrada, -100, 100);
  int magnitud = abs(entrada);

  if (magnitud <= zonaMuerta) return 0;

  magnitud = map(magnitud, zonaMuerta + 1, 100, 1, 100);
  long cuadratica = (long)magnitud * magnitud / 100L;
  long salida = ((long)magnitud * (100 - curvaExpo) + cuadratica * curvaExpo) / 100L;

  return entrada < 0 ? -(int)salida : (int)salida;
}

int porcentajeAPwm(int porcentaje, int trim) {
  if (porcentaje == 0) return 0;

  int magnitud = abs(porcentaje);
  int pwm = map(magnitud, 1, 100, pwmMinimo, pwmMaximo);
  pwm = constrain(pwm + trim, 0, pwmMaximo);
  return porcentaje < 0 ? -pwm : pwm;
}

// ---------------------- RAMPAS Y MOTORES ------------------
void actualizarRampas(unsigned long ahora) {
  if (ahora - ultimoPasoRampa < INTERVALO_RAMPA_MS) return;
  ultimoPasoRampa = ahora;

  salidaIzquierda = aproximarSalida(salidaIzquierda, objetivoIzquierdo);
  salidaDerecha = aproximarSalida(salidaDerecha, objetivoDerecho);

  aplicarMotorIzquierdo(salidaIzquierda);
  aplicarMotorDerecho(salidaDerecha);
}

int aproximarSalida(int actual, int objetivo) {
  int objetivoTemporal = objetivo;

  // Nunca invertir directamente: primero pasar por cero.
  if ((actual > 0 && objetivo < 0) || (actual < 0 && objetivo > 0)) {
    objetivoTemporal = 0;
  }

  int paso = abs(objetivoTemporal) < abs(actual) ? PASO_FRENADO : PASO_ACELERACION;

  if (actual < objetivoTemporal) return min(actual + paso, objetivoTemporal);
  if (actual > objetivoTemporal) return max(actual - paso, objetivoTemporal);
  return actual;
}

void aplicarMotorIzquierdo(int pwmFirmado) {
  int pwm = constrain(abs(pwmFirmado), 0, 255);
  analogWrite(ENA, pwm);
  digitalWrite(IN1, pwmFirmado > 0 ? HIGH : LOW);
  digitalWrite(IN2, pwmFirmado < 0 ? HIGH : LOW);
}

void aplicarMotorDerecho(int pwmFirmado) {
  int pwm = constrain(abs(pwmFirmado), 0, 255);
  analogWrite(ENB, pwm);
  digitalWrite(IN3, pwmFirmado > 0 ? HIGH : LOW);
  digitalWrite(IN4, pwmFirmado < 0 ? HIGH : LOW);
}

void detenerInmediato() {
  ejeDireccion = 0;
  ejeAcelerador = 0;
  objetivoIzquierdo = 0;
  objetivoDerecho = 0;
  salidaIzquierda = 0;
  salidaDerecha = 0;
  controlRecibido = false;

  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// ---------------------- AUTO SIMPLE -----------------------
void alternarModo(bool desdeBT) {
  detenerInmediato();

  if (modoControl == MODO_MANUAL) {
    modoControl = MODO_AUTO_SIMPLE;
    estadoAuto = AUTO_AVANZAR;
    inicioEstadoAuto = millis();
    responder(F("OK;MODO;AUTO_SIMPLE"), desdeBT);
  } else {
    modoControl = MODO_MANUAL;
    responder(F("OK;MODO;MANUAL"), desdeBT);
  }
}

void actualizarAutomatico(unsigned long ahora) {
  switch (estadoAuto) {
    case AUTO_AVANZAR:
      fijarObjetivosPorcentaje(55, 55);
      if (distanciaCm > 0 && distanciaCm <= DISTANCIA_AUTO_CM) {
        estadoAuto = AUTO_RETROCEDER;
        inicioEstadoAuto = ahora;
        giroAutoIzquierda = random(0, 2) == 0;
      }
      break;

    case AUTO_RETROCEDER:
      fijarObjetivosPorcentaje(-55, -55);
      if (ahora - inicioEstadoAuto >= 350UL) {
        estadoAuto = AUTO_GIRAR;
        inicioEstadoAuto = ahora;
      }
      break;

    case AUTO_GIRAR:
      if (giroAutoIzquierda) fijarObjetivosPorcentaje(-50, 50);
      else fijarObjetivosPorcentaje(50, -50);

      if (ahora - inicioEstadoAuto >= 450UL) {
        estadoAuto = AUTO_AVANZAR;
        inicioEstadoAuto = ahora;
      }
      break;
  }
}

void fijarObjetivosPorcentaje(int izquierda, int derecha) {
  objetivoIzquierdo = porcentajeAPwm(izquierda, trimIzquierdo);
  objetivoDerecho = porcentajeAPwm(derecha, trimDerecho);
}

// ------------------------ HC-SR04 -------------------------
void actualizarUltrasonico(unsigned long ahora) {
  unsigned long intervalo = modoControl == MODO_MANUAL
    ? INTERVALO_US_MANUAL_MS : INTERVALO_US_AUTO_MS;

  if (ahora - ultimoUltrasonico < intervalo) return;
  ultimoUltrasonico = ahora;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duracion = pulseIn(ECHO_PIN, HIGH, 12000UL);
  distanciaCm = duracion == 0 ? -1 : duracion / 58;
}

// ----------------------- CALIBRACION ----------------------
void configurar(const char* parametro, int valor, bool desdeBT) {
  bool valido = true;

  if (igual(parametro, "SENS")) sensibilidad = constrain(valor, 20, 100);
  else if (igual(parametro, "TURN")) sensibilidadGiro = constrain(valor, 20, 150);
  else if (igual(parametro, "DEAD")) zonaMuerta = constrain(valor, 0, 30);
  else if (igual(parametro, "MAX")) pwmMaximo = constrain(valor, 80, 255);
  else if (igual(parametro, "MIN")) pwmMinimo = constrain(valor, 0, 150);
  else if (igual(parametro, "EXPO")) curvaExpo = constrain(valor, 0, 100);
  else if (igual(parametro, "TRIML")) trimIzquierdo = constrain(valor, -30, 30);
  else if (igual(parametro, "TRIMR")) trimDerecho = constrain(valor, -30, 30);
  else valido = false;

  if (pwmMinimo > pwmMaximo) pwmMinimo = pwmMaximo;
  calcularObjetivos();

  if (valido) enviarConfiguracion(desdeBT);
  else responder(F("ERR;CFG"), desdeBT);
}

void enviarConfiguracion(bool desdeBT) {
  char mensaje[118];
  snprintf(mensaje, sizeof(mensaje),
    "CFG;SENS=%d;TURN=%d;DEAD=%d;MAX=%d;MIN=%d;EXPO=%d;TRIML=%d;TRIMR=%d",
    sensibilidad, sensibilidadGiro, zonaMuerta, pwmMaximo, pwmMinimo,
    curvaExpo, trimIzquierdo, trimDerecho);
  responderTexto(mensaje, desdeBT);
}

void enviarAyuda(bool desdeBT) {
  responder(F("ROBOLINK;x/y:0..200,p:80..255,g:20..150,m/s:0/1"), desdeBT);
  responder(F("CMD;J,x,y | X,x | Y,y | F/V/I/D/S | M"), desdeBT);
  responder(F("CMD;CFG,SENS|TURN|DEAD|MAX|MIN|EXPO|TRIML|TRIMR,valor"), desdeBT);
  responder(F("CMD;STATUS | PING | RX | ?"), desdeBT);
  responder(F("SAR;r=proteccion sensores 0/1"), desdeBT);
}

// Telemetria solo por USB para no bloquear la recepcion del HC-06.
void enviarEstadoUSB(unsigned long ahora) {
  if (ahora - ultimoEstadoUSB < 500UL) return;
  ultimoEstadoUSB = ahora;

  Serial.print(F("TEL;mode="));
  Serial.print(modoControl == MODO_MANUAL ? F("MANUAL") : F("AUTO"));
  Serial.print(F(";x="));
  Serial.print(ejeDireccion);
  Serial.print(F(";y="));
  Serial.print(ejeAcelerador);
  Serial.print(F(";left="));
  Serial.print(salidaIzquierda);
  Serial.print(F(";right="));
  Serial.print(salidaDerecha);
  Serial.print(F(";dist="));
  Serial.print(distanciaCm);
  Serial.print(F(";prot=")); Serial.print(proteccion);
  Serial.print(F(";IR=")); Serial.print(irIzq); Serial.print(','); Serial.print(irDer);
  Serial.print(F(";block=")); Serial.print(bloqueoSensor);
  Serial.print(F(";rx=")); Serial.print(bytesRX);
  Serial.print(F(";cmd=")); Serial.print(controlesRX);
  Serial.print(F(";err=")); Serial.println(erroresRX);
}



bool campoSAR(const char* clave, int valor) {
  if (igual(clave, "r")) {
    if (valor == 0 || valor == 1) proteccion = valor;
    return true;
  }
  return false;
}

void protegerSensores() {
  irIzq = digitalRead(IR_IZQ) == LOW;
  irDer = digitalRead(IR_DER) == LOW;
  bool adelante = objetivoIzquierdo > 0 || objetivoDerecho > 0;
  bloqueoSensor = proteccion && (objetivoIzquierdo || objetivoDerecho) &&
    (irIzq || irDer || (adelante && (distanciaCm < 0 || distanciaCm <= 20)));
  if (bloqueoSensor) objetivoIzquierdo = objetivoDerecho = 0;
}
