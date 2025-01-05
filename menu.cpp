#include "menu.h"
#include "display.h"
#include "button.h"
#include <TFT_eSPI.h>

int menuIndex = -1;

#define MAX_STACK_SIZE 10

struct MenuStack {
    Menu* menus[MAX_STACK_SIZE];
    int top = -1;
} menuStack;

void pushMenu(Menu* menu) {
  menuIndex = -1;
  if (menuStack.top < MAX_STACK_SIZE - 1) {
      menuStack.menus[++menuStack.top] = menu;
      getDisplay()->fillScreen(TFT_BLACK);
  } else {
      //debugln("Stack overflow!");
  }
}

int getMenuIndex() {
  return menuIndex;
}

Menu* topMenu() {
    return menuStack.menus[menuStack.top];
}

void popMenu() {
  menuIndex = -1;
  if (menuStack.top > 0) {
      menuStack.top--;
      //getDisplay()->fillScreen(TFT_BLACK);
      drawBG();
  } else {
      //debugln("Stack underflow!");
  }
  //debugln("Current menu is:" + topMenu()->name);
}

void displayMenu() {
  ButtonState button = getButtonState();
  bool changed = button != BUTTON_NO;
  Menu* currentMenu = topMenu();
  //debugln("Rendering menu: " + currentMenu->name);
  if (!changed) {
    //debugln("Nothing changed no rendering");
    return;
  }
  if (button == BUTTON_DOWN) {
    beginRender();
    if (menuIndex < currentMenu->size - 1) menuIndex++;
  } else if (button == BUTTON_UP) {
    button = BUTTON_RENDER;
    if (menuIndex > 0) menuIndex--;
  } else if (button == BUTTON_OK && menuIndex > -1) {
    beginRender();
    //debugln(currentMenu->items[menuIndex].name + " SELECTED"); 
    if (currentMenu->items[menuIndex].selectable) {
      currentMenu->items[menuIndex].action();
      menuIndex = -1;
      return;
    }
  } else if (button == BUTTON_CANCEL || button == BUTTON_LONG_PRESS) {
    beginRender();
    //debugln("CANCEL"); 
    popMenu();
    topMenu()->render();
    return;
  }
  //debugln("Clear display");
  TFT_eSPI* display = getDisplay();
  // Clear the buffer.
  display->fillScreen(TFT_BLACK);
  // Display Text
  display->setTextSize(1);

  int minVisible = 0;
  int maxVisible = currentMenu->size;

  if (maxVisible > MENU_MAX_LINE_PER_PAGE - 1) {
    minVisible = max(menuIndex - MENU_MAX_LINE_PER_PAGE / 2, 0);
    maxVisible = min(minVisible + MENU_MAX_LINE_PER_PAGE, maxVisible);
  }

  // Print each menu item to the serial monitor
  for (int i = minVisible; i < maxVisible; i++) {
    String name = currentMenu->items[i].name;
    //debugln("Rendering item: " + name);
    display->setTextColor(((252 >> 3) << 11) | ((3 >> 2) << 5) | (3 >> 3), TFT_BLACK);
    if (i == menuIndex) {
      display->setTextColor(TFT_YELLOW, TFT_BLACK); // 'inverted' text
    }
    bool selectable = currentMenu->items[i].selectable;
    display->setCursor(0, MENU_MIN_Y_POS + 22 * (i - minVisible));
    if (selectable) display->println(name);
    else {
      display->setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      display->print("\"");
      display->print(name);
      display->println("\"");
    }
  }

  //display->display();
  endRender();
}

void disabledItem() {
  endRender();
}