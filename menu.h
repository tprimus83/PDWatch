#include <Arduino.h>

#ifndef MENU_H
#define MENU_H

const int MAX_MENU_ITEM_SIZE = 20;
#define MENU_MIN_Y_POS 20
#define MENU_MAX_LINE_PER_PAGE 6

struct MenuItem {
    String name;
    void (*action)();
    bool selectable = true;
};

struct Menu {
    String name;
    MenuItem* items;
    int size;
    void (*render)();
};

void displayMenu();
void pushMenu(Menu* menu);
Menu* topMenu();
void popMenu();
int getMenuIndex();
void disabledItem();

#endif