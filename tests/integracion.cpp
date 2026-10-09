#include "../firmware/SAR/SAR.ino"
void comprobar(bool ok, byte caso) {
  if (!ok) { GPIOR1 = caso; GPIOR0 = 0xEE; for (;;) {} }
}
void entrada(const char* texto) {
  char copia[128]; strcpy(copia, texto); procesarLinea(copia, true);
}
int main() {
  static_assert(ENA==9 && ENB==10 && IN1==4 && IN2==5 && IN3==6 && IN4==7, "L298N");
  static_assert(BT_RX==13 && BT_TX==12 && TRIG_PIN==2 && ECHO_PIN==3, "BT/US");
  static_assert(IR_IZQ==8 && IR_DER==11 && BAUD_BT==38400, "IR/baud");
  static_assert(INTERVALO_RAMPA_MS==10, "Respuesta");
  setup(); reloj=100;
  entrada("x:42,y:100,p:160,g:100,m:0,s:0");
  comprobar(ejeDireccion==-58 && objetivoIzquierdo==-106 && objetivoDerecho==106, 1);
  entrada("x:69"); comprobar(ejeDireccion==-31 && objetivoIzquierdo==-81, 2);
  entrada("steer:200,throttle:100,power:160,turn:100,unused_left:7");
  comprobar(objetivoIzquierdo==160 && objetivoDerecho==-160, 3);
  entrada(" x:100.00, y:200.00, p:160, g:100");
  comprobar(ejeDireccion==0 && ejeAcelerador==100 && objetivoIzquierdo==160, 4);
  entrada("J,-60,0"); comprobar(objetivoIzquierdo==-108 && objetivoDerecho==108, 5);
  entrada("X,0"); entrada("Y,100");
  comprobar(objetivoIzquierdo==160 && objetivoDerecho==160, 6);
  distanciaCm=4; protegerSensores();
  comprobar(bloqueoSensor && objetivoIzquierdo==0, 7);
  entrada("r:0"); calcularObjetivos(); protegerSensores();
  comprobar(!bloqueoSensor && objetivoIzquierdo==160, 8);
  entrada("r:1"); distanciaCm=100; pins[IR_IZQ]=LOW;
  calcularObjetivos(); protegerSensores();
  comprobar(bloqueoSensor && objetivoIzquierdo==0, 9);
  pins[IR_IZQ]=HIGH; pins[IR_DER]=LOW;
  entrada("J,0,-100"); protegerSensores();
  comprobar(bloqueoSensor && objetivoDerecho==0, 10);
  pins[IR_DER]=HIGH; distanciaCm=-1;
  entrada("J,0,100"); protegerSensores();
  comprobar(bloqueoSensor && objetivoIzquierdo==0, 11);
  entrada("J,0,-100"); protegerSensores();
  comprobar(!bloqueoSensor && objetivoIzquierdo<0, 12);
  entrada("s:1");
  comprobar(!controlRecibido && salidaIzquierda==0 && salidaDerecha==0, 13);
  entrada("J,0,100"); reloj+=300; loop();
  comprobar(!controlRecibido && objetivoIzquierdo==0, 14);
  entrada("p:120"); comprobar(objetivoIzquierdo==0 && objetivoDerecho==0, 15);
  int pwm=0;
  for (byte i=0;i<8;++i) pwm=aproximarSalida(pwm,70);
  comprobar(pwm==64 && aproximarSalida(pwm,70)==70, 16);
  pwm=0;
  for (byte i=0;i<20;++i) pwm=aproximarSalida(pwm,160);
  comprobar(pwm==160, 17);
  for (byte i=0;i<14;++i) {
    pwm=aproximarSalida(pwm,-160); comprobar(pwm>=0, 18);
  }
  comprobar(pwm==0 && aproximarSalida(pwm,-160)==-8, 19);
  salidaIzquierda=salidaDerecha=0; objetivoIzquierdo=objetivoDerecho=160;
  ultimoPasoRampa=100;
  actualizarRampas(109); comprobar(salidaIzquierda==0, 20);
  actualizarRampas(110); comprobar(salidaIzquierda==8 && salidaDerecha==8, 21);
  GPIOR0=0xA5; for (;;) {}
}
