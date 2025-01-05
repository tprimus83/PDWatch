#include "button.h"

ButtonState buttonState = BUTTON_NO;
ButtonState button = BUTTON_NO;

ButtonState getButtonState() {
    return button;
}

void beginRender() {
    button = BUTTON_RENDER;
}
void endRender() {
    button = BUTTON_NO;
}
void buttonHandled() {
    button = BUTTON_NO;
}

void setButtonState(ButtonState newState) {
  button = newState;
}