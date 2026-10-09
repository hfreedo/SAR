#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
typedef uint8_t byte;
class __FlashStringHelper;
#define F(x) reinterpret_cast<const __FlashStringHelper*>(x)
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define A0 14
#define A1 15
#define A2 16
#define constrain(v,l,h) ((v)<(l)?(l):((v)>(h)?(h):(v)))
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
unsigned long reloj = 0;
int pins[20] = {};
unsigned long millis() { return reloj; }
void pinMode(int p,int m) { if (m == INPUT_PULLUP) pins[p] = HIGH; }
void analogWrite(int,int) {}
void digitalWrite(int p,int v) { pins[p] = v; }
int digitalRead(int p) { return pins[p]; }
int analogRead(int) { return 0; }
void randomSeed(unsigned long) {}
long random(long a,long) { return a; }
long map(long v,long a,long b,long c,long d) { return (v-a)*(d-c)/(b-a)+c; }
void delayMicroseconds(int) {}
unsigned long pulseIn(int,int,unsigned long) { return 5800; }
class Stream {
  char datos[128] = {};
  int cursor = 0;
public:
  void begin(unsigned long) {}
  void feed(const char* s) { strcpy(datos,s); cursor=0; }
  int available() { return datos[cursor] != 0; }
  char read() { return datos[cursor++]; }
  template<typename T> void print(T) {}
  template<typename T> void println(T) {}
};
class SoftwareSerial : public Stream { public: SoftwareSerial(int,int) {} };
Stream Serial;
