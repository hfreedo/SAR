#pragma once
class ServoTimer2Plus {
public:
  int valor=90;
  void attach(int,int,int) {}
  void write(int v) { valor=v; }
};
