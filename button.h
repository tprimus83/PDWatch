#include <Arduino.h>

#ifndef BUTTON_H
#define BUTTON_H


enum ButtonState {
    BUTTON_NO = 'N',
    BUTTON_UP = 'U',
    BUTTON_DOWN = 'D',
    BUTTON_OK = 'O',
    BUTTON_CANCEL = 'C',
    BUTTON_RENDER = 'R',
    BUTTON_PRESS = 'P', //Press
    BUTTON_LONG_PRESS = 'A' //Accept
};

ButtonState getButtonState();
void setButtonState(ButtonState newState);
void beginRender();
void endRender();
void buttonHandled();
#endif