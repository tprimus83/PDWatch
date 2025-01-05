#ifndef DISPLAY_H
#define DISPLAY_H
#include <TFT_eSPI.h> 

void alert(String text);
void beginDisplay();
TFT_eSPI* getDisplay();
void drawBG();
#endif