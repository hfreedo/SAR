#define TEST_LIGHTS 2
#include "../firmware/SAR/SAR.ino"
void comprobar(bool ok,byte caso) {
  if (!ok) { GPIOR1=caso; GPIOR0=0xEE; for (;;) {} }
}
void entrada(const char* s) {
  char copia[128]; strcpy(copia,s); procesarLinea(copia,true);
}
int main() {
  static_assert(ENA==9 && ENB==10 && IN1==4 && IN2==5 && IN3==6 && IN4==7,"L298N");
#if TEST_LIGHTS == 2
  static_assert(BT_RX==13 && BT_TX==12 && TRIG_PIN==2 && ECHO_PIN==3,"BT/US nuevos");
#else
  static_assert(BT_RX==2 && BT_TX==3 && TRIG_PIN==12 && ECHO_PIN==13,"BT/US");
#endif
  static_assert(IR_IZQ==8 && IR_DER==11 && BAUD_BT==38400,"IR/baud");
  setup(); reloj=100;
  comprobar(!servo1Activo && !servo2Activo && !luces,1);
  entrada("x:42,y:100,p:160,g:100,m:0,s:0");
  comprobar(ejeDireccion==-58 && ejeAcelerador==0 && objetivoIzquierdo==-106 && objetivoDerecho==106,2);
  entrada("x:69"); comprobar(ejeDireccion==-31 && objetivoIzquierdo==-81,3);
  entrada("steer:200,throttle:100,power:160,turn:100,unused_left:7");
  comprobar(objetivoIzquierdo==160 && objetivoDerecho==-160,4);
  entrada(" x:100.00, y:200.00, p:160, g:100");
  comprobar(ejeDireccion==0 && ejeAcelerador==100 && objetivoIzquierdo==160,5);
  entrada("J,-60,0"); comprobar(objetivoIzquierdo==-108 && objetivoDerecho==108,6);
  entrada("X,0"); entrada("Y,100"); comprobar(objetivoIzquierdo==160 && objetivoDerecho==160,7);
  distanciaCm=4; protegerSensores(); comprobar(bloqueoSensor && objetivoIzquierdo==0,8);
  entrada("r:0"); calcularObjetivos(); protegerSensores(); comprobar(!bloqueoSensor && objetivoIzquierdo==160,9);
  entrada("r:1"); distanciaCm=100; pins[IR_IZQ]=LOW; calcularObjetivos(); protegerSensores();
  comprobar(bloqueoSensor && objetivoIzquierdo==0,10);
  pins[IR_IZQ]=HIGH;
  entrada("u:120,w:60"); comprobar(servo1Activo && servo2Activo,11);
  actualizarServos(reloj); comprobar(posicion1==91 && posicion2==89,12);
  entrada("s:1"); comprobar(!controlRecibido && !salidaIzquierda && destino1==posicion1,13);
  entrada("l:1,n:1,v:10"); comprobar(animacion==2 && velocidadLED[1]==10,14);
  entrada("n:1"); comprobar(animacion==2,15);
  entrada("n:0"); entrada("n:1"); comprobar(animacion==3,16);
  reloj=1000; actualizarLuces(reloj);
#if TEST_LIGHTS
  comprobar(framesLED==1 && luzEnviada,17);
  entrada("l:0"); actualizarLuces(reloj);
  comprobar(framesLED==2 && !luzEnviada,18);
  for(int i=0;i<100;++i) actualizarLuces(reloj+=100);
  comprobar(framesLED==2,19);
#else
  comprobar(framesLED==0,20);
#endif
  entrada("J,0,100"); reloj+=300; loop();
  comprobar(!controlRecibido && objetivoIzquierdo==0,21);
  comprobar(aproximarSalida(2,-160)==0,22);
#if TEST_LIGHTS
  static_assert(INTERVALO_RAMPA_MS == 10, "Intervalo de respuesta");
  int pwm = 0;
  for (byte i=0;i<8;++i) pwm=aproximarSalida(pwm,70);
  comprobar(pwm==64 && aproximarSalida(pwm,70)==70,23);
  pwm=0;
  for (byte i=0;i<20;++i) pwm=aproximarSalida(pwm,160);
  comprobar(pwm==160,24);
  for (byte i=0;i<14;++i) {
    pwm=aproximarSalida(pwm,-160);
    comprobar(pwm>=0,25);
  }
  comprobar(pwm==0 && aproximarSalida(pwm,-160)==-8,26);
  pwm=80;
  for (byte i=0;i<20;++i) pwm=aproximarSalida(pwm,17);
  comprobar(pwm==17,27);
  salidaIzquierda=salidaDerecha=0;
  objetivoIzquierdo=objetivoDerecho=160;
  ultimoPasoRampa=100;
  actualizarRampas(109); comprobar(salidaIzquierda==0,28);
  actualizarRampas(110); comprobar(salidaIzquierda==8 && salidaDerecha==8,29);
#endif
  GPIOR0=0xA5; for (;;) {}
}
